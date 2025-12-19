#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_PROCS 100
#define MAX_COMMAND_LEN 256
#define MAX_ARGS 20

// Структура для хранения информации о процессе
typedef struct {
    pid_t pid;
    char command[MAX_COMMAND_LEN];
} ProcessInfo;

ProcessInfo proc_stack[MAX_PROCS];
int proc_top = -1;

// Прототипы функций
void push_process(pid_t pid, const char *cmd);
pid_t pop_process();
void list_processes();
void sigint_handler(int sig);
void builtin_ls();
void builtin_cat(char *filename);
void builtin_nice(int prio);
void builtin_killall(char *name);
void builtin_help();
void run_external_command(char **args, int background);
void run_command(char *cmd);
void trim_whitespace(char *str);
void wait_for_children();

// Добавление процесса в стек LIFO
void push_process(pid_t pid, const char *cmd) {
    if (proc_top < MAX_PROCS - 1) {
        proc_top++;
        proc_stack[proc_top].pid = pid;
        if (cmd) {
            strncpy(proc_stack[proc_top].command, cmd, MAX_COMMAND_LEN - 1);
            proc_stack[proc_top].command[MAX_COMMAND_LEN - 1] = '\0';
        } else {
            strcpy(proc_stack[proc_top].command, "unknown");
        }
        printf("[%d] Запущен процесс: %s (PID: %d)\n", proc_top, 
               proc_stack[proc_top].command, pid);
    } else {
        printf("Достигнут максимальный лимит процессов (%d)\n", MAX_PROCS);
    }
}

// Извлечение процесса из стека (LIFO)
pid_t pop_process() {
    if (proc_top >= 0) {
        pid_t pid = proc_stack[proc_top].pid;
        char *cmd = proc_stack[proc_top].command;
        printf("Завершение процесса: %s (PID: %d)\n", cmd, pid);
        proc_top--;
        return pid;
    }
    return -1;
}

// Отображение всех активных процессов
void list_processes() {
    printf("\n=== Активные процессы (всего: %d) ===\n", proc_top + 1);
    for (int i = proc_top; i >= 0; i--) {
        printf("[%d] PID: %d, Команда: %s\n", 
               i, proc_stack[i].pid, proc_stack[i].command);
    }
    printf("=================================\n");
}

// Ожидание завершения дочерних процессов
void wait_for_children() {
    pid_t pid;
    int status;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Удаляем завершенный процесс из стека
        for (int i = 0; i <= proc_top; i++) {
            if (proc_stack[i].pid == pid) {
                printf("Процесс %d завершился: %s\n", pid, proc_stack[i].command);
                
                // Сдвигаем стек
                for (int j = i; j < proc_top; j++) {
                    proc_stack[j] = proc_stack[j + 1];
                }
                proc_top--;
                break;
            }
        }
    }
}

// Обработчик сигнала SIGINT (CTRL+C)
void sigint_handler(int sig) {
    printf("\nПолучен сигнал SIGINT (CTRL+C)\n");
    
    if (proc_top >= 0) {
        pid_t pid = pop_process();
        if (pid > 0) {
            printf("Отправка SIGTERM процессу %d\n", pid);
            if (kill(pid, SIGTERM) == 0) {
                // Даем процессу время на корректное завершение
                sleep(1);
                // Если процесс еще жив, отправляем SIGKILL
                if (kill(pid, 0) == 0) {
                    printf("Процесс не ответил на SIGTERM, отправляю SIGKILL\n");
                    kill(pid, SIGKILL);
                }
                waitpid(pid, NULL, 0);
            } else {
                perror("Ошибка при завершении процесса");
            }
        }
    } else {
        printf("Нет активных процессов для завершения\n");
    }
    
    // Выводим приглашение на новой строке
    printf("tosh> ");
    fflush(stdout);
}

// Команда ls
void builtin_ls() {
    DIR *d = opendir(".");
    if (!d) {
        perror("opendir");
        return;
    }
    
    struct dirent *dir;
    int count = 0;
    printf("Содержимое текущей директории:\n");
    
    while ((dir = readdir(d)) != NULL) {
        // Пропускаем скрытые файлы (начинающиеся с .)
        if (dir->d_name[0] != '.') {
            printf("%-20s", dir->d_name);
            count++;
            if (count % 4 == 0) printf("\n");
        }
    }
    
    if (count % 4 != 0) printf("\n");
    printf("Всего: %d файлов/директорий\n", count);
    
    closedir(d);
}

// Команда cat
void builtin_cat(char *filename) {
    if (!filename) {
        printf("Использование: cat <имя_файла>\n");
        return;
    }
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return;
    }
    
    printf("Содержимое файла '%s':\n", filename);
    printf("================================\n");
    
    char line[1024];
    int line_num = 1;
    
    while (fgets(line, sizeof(line), f)) {
        printf("%4d: %s", line_num++, line);
    }
    
    printf("================================\n");
    fclose(f);
}

// Команда nice
void builtin_nice(int prio) {
    // Ограничиваем приоритет в допустимых пределах
    if (prio < -20) prio = -20;
    if (prio > 19) prio = 19;
    
    if (setpriority(PRIO_PROCESS, 0, prio) < 0) {
        perror("setpriority");
    } else {
        printf("Приоритет текущего процесса изменен на %d\n", prio);
        printf("Текущий приоритет: %d\n", getpriority(PRIO_PROCESS, 0));
    }
}

// Команда killall
void builtin_killall(char *name) {
    if (!name) {
        printf("Использование: killall <имя_процесса>\n");
        return;
    }
    
    printf("Поиск процессов с именем '%s'...\n", name);
    
    DIR *proc = opendir("/proc");
    if (!proc) {
        perror("opendir /proc");
        return;
    }
    
    struct dirent *entry;
    int killed = 0;
    
    while ((entry = readdir(proc)) != NULL) {
        if (entry->d_type == DT_DIR) {
            int pid = atoi(entry->d_name);
            if (pid <= 0) continue;
            
            char cmdpath[256];
            snprintf(cmdpath, sizeof(cmdpath), "/proc/%d/comm", pid);
            
            FILE *cmdf = fopen(cmdpath, "r");
            if (cmdf) {
                char pname[256];
                if (fgets(pname, sizeof(pname), cmdf)) {
                    pname[strcspn(pname, "\n")] = 0;
                    if (strcmp(pname, name) == 0) {
                        if (kill(pid, SIGTERM) == 0) {
                            printf("  Завершен процесс: %s (PID: %d)\n", pname, pid);
                            killed++;
                        } else {
                            printf("  Не удалось завершить процесс: %s (PID: %d)\n", pname, pid);
                        }
                    }
                }
                fclose(cmdf);
            }
        }
    }
    
    closedir(proc);
    printf("Всего завершено процессов: %d\n", killed);
}

// Команда help
void builtin_help() {
    printf("\n=== Доступные команды ===\n");
    printf("ls               - список файлов в текущей директории\n");
    printf("cat <файл>       - вывод содержимого файла\n");
    printf("nice [приоритет] - изменение приоритета процесса\n");
    printf("killall <имя>    - завершение всех процессов с указанным именем\n");
    printf("browser          - запуск браузера Google Chrome\n");
    printf("ps               - список активных процессов\n");
    printf("help             - вывод этой справки\n");
    printf("exit             - выход из терминала\n");
    printf("<команда> &      - запуск команды в фоновом режиме\n");
    printf("<любая команда>  - запуск внешней программы\n");
    printf("==================================\n");
}

// Запуск внешней команды
void run_external_command(char **args, int background) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "Ошибка: команда '%s' не найдена\n", args[0]);
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        // Родительский процесс
        push_process(pid, args[0]);
        
        if (!background) {
            // Если команда не фоновая, ждем завершения
            int status;
            waitpid(pid, &status, 0);
            
            // Удаляем процесс из стека после завершения
            for (int i = 0; i <= proc_top; i++) {
                if (proc_stack[i].pid == pid) {
                    for (int j = i; j < proc_top; j++) {
                        proc_stack[j] = proc_stack[j + 1];
                    }
                    proc_top--;
                    break;
                }
            }
            
            if (WIFEXITED(status)) {
                printf("Процесс завершился с кодом %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Процесс завершен сигналом %d\n", WTERMSIG(status));
            }
        } else {
            printf("Процесс запущен в фоновом режиме (PID: %d)\n", pid);
        }
    } else {
        perror("fork");
    }
}

// Удаление лишних пробелов
void trim_whitespace(char *str) {
    char *end;
    
    // Убираем пробелы в начале
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return;
    
    // Убираем пробелы в конце
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Записываем новый конец строки
    *(end + 1) = 0;
}

// Обработка и выполнение команды
void run_command(char *cmd) {
    char *args[MAX_ARGS + 1];
    int i = 0;
    int background = 0;
    
    // Убираем лишние пробелы
    trim_whitespace(cmd);
    
    // Пропускаем пустые команды
    if (strlen(cmd) == 0) return;
    
    // Проверяем на фоновый режим
    char *bg_pos = strchr(cmd, '&');
    if (bg_pos != NULL) {
        *bg_pos = '\0';
        background = 1;
        trim_whitespace(cmd);
    }
    
    // Разбиваем команду на аргументы
    char *token = strtok(cmd, " \t\n");
    while (token != NULL && i < MAX_ARGS) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    
    if (!args[0]) return;
    
    // Проверяем завершенные процессы перед выполнением новой команды
    wait_for_children();
    
    // Обработка встроенных команд
    if (strcmp(args[0], "ls") == 0) {
        builtin_ls();
    } else if (strcmp(args[0], "cat") == 0) {
        builtin_cat(args[1]);
    } else if (strcmp(args[0], "nice") == 0) {
        int prio = args[1] ? atoi(args[1]) : 0;
        builtin_nice(prio);
    } else if (strcmp(args[0], "killall") == 0) {
        builtin_killall(args[1]);
    } else if (strcmp(args[0], "browser") == 0) {
        // Специальная обработка для браузера
        char *browser_cmd[] = {"xdg-open", "https://www.google.com", NULL};
        run_external_command(browser_cmd, background);
    } else if (strcmp(args[0], "ps") == 0) {
        list_processes();
    } else if (strcmp(args[0], "help") == 0) {
        builtin_help();
    } else if (strcmp(args[0], "exit") == 0) {
        printf("Выход из терминала\n");
        // Завершаем все процессы перед выходом
        while (proc_top >= 0) {
            pid_t pid = pop_process();
            if (pid > 0) {
                kill(pid, SIGTERM);
            }
        }
        exit(0);
    } else {
        // Запуск внешней команды
        run_external_command(args, background);
    }
}

int main() {
    // Устанавливаем обработчик сигналов
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    
    // Игнорируем SIGTTOU чтобы избежать проблем с управлением терминалом
    signal(SIGTTOU, SIG_IGN);
    
    char command[MAX_COMMAND_LEN];
    
    printf("\n");
    printf("==========================================\n");
    printf("      Добро пожаловать в TOSH v1.0\n");
    printf("  (Terminal Operating Shell with LIFO)\n");
    printf("==========================================\n");
    printf("Для справки введите 'help'\n");
    printf("Для выхода введите 'exit'\n\n");
    
    // Основной цикл терминала
    while (1) {
        printf("tosh> ");
        fflush(stdout);
        
        if (!fgets(command, sizeof(command), stdin)) {
            printf("\n");
            break;
        }
        
        // Убираем символ новой строки
        command[strcspn(command, "\n")] = 0;
        
        // Выполняем команду
        run_command(command);
    }
    
    // Завершение всех процессов перед выходом
    while (proc_top >= 0) {
        pid_t pid = pop_process();
        if (pid > 0) {
            kill(pid, SIGTERM);
        }
    }
    
    printf("Терминал завершен. До свидания!\n");
    return 0;
}
