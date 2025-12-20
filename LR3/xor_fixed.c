#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define FIFO1 "fifo1"
#define FIFO2 "fifo2"
#define RESULT_FIFO "result_fifo"

// Функция создания именованного канала (FIFO)
void create_fifo(const char *fifo_name) {
    // Создание FIFO с правами доступа 0666 (чтение и запись для всех)
    // EEXIST - канал уже существует (не совсем ошибка, это означает что канал уже используется кем-то)
    if (mkfifo(fifo_name, 0666) == -1 && errno != EEXIST) {
        perror("Ошибка создания именованного канала");
        exit(1);
    }
}

// Функция чтения данных из именованного канала
void read_from_fifo(const char *fifo_name, char **data, size_t *size) {
    // Открытие канала для чтения (блокируется до появления источника данных)
    int fd = open(fifo_name, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия именованного канала для чтения");
        exit(1);
    }

    // Чтение размера данных (первое что отправляет источник)
    size_t data_size;
    if (read(fd, &data_size, sizeof(data_size)) != sizeof(data_size)) {
        perror("Ошибка чтения размера данных");
        close(fd);
        exit(1);
    }

    // Выделение памяти под данные
    *data = malloc(data_size);
    if (*data == NULL) {
        perror("Ошибка выделения памяти");
        close(fd);
        exit(1);
    }

    // Чтение данных из канала
    size_t total_read = 0;
    while (total_read < data_size) {
        ssize_t bytes_read = read(fd, *data + total_read, data_size - total_read);
        if (bytes_read <= 0) {
            perror("Ошибка чтения данных из канала");
            break;
        }
        total_read += bytes_read;
    }

    *size = data_size;
    close(fd);
}

// Функция записи результата в файл
void write_result(const char *filename, const char *data, size_t size) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Ошибка открытия выходного файла");
        exit(1);
    }

    // Запись всех данных в файл
    if (write(fd, data, size) != (ssize_t)size) {
        perror("Ошибка записи в файл");
        close(fd);
        exit(1);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    // Проверка аргументов командной строки
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <file1> <file2> <output_file>\n", argv[0]);
        exit(1);
    }

    const char *file1 = argv[1];
    const char *file2 = argv[2];
    const char *output_file = argv[3];

    // Создание именованных каналов для межпроцессного взаимодействия
    create_fifo(FIFO1);
    create_fifo(FIFO2);
    create_fifo(RESULT_FIFO);

    // Запуск первого процесса reader для обработки file1
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // В дочернем процессе: замена образа процесса на программу reader
        execl("./sender_fixed", "sender_fixed", file1, FIFO1, NULL);
        perror("Ошибка запуска reader для file1");
        exit(1);
    }

    // Запуск второго процесса reader для обработки file2
    pid_t pid2 = fork();
    if (pid2 == 0) {
        execl("./sender_fixed", "sender_fixed", file2, FIFO2, NULL);
        perror("Ошибка запуска reader для file2");
        exit(1);
    }

    // Чтение данных из обоих каналов (блокируется до появления данных)
    char *data1, *data2;
    size_t size1, size2;

    read_from_fifo(FIFO1, &data1, &size1);
    read_from_fifo(FIFO2, &data2, &size2);

    // Определение минимального размера для операции XOR
    size_t max_size = (size1 > size2) ? size1 : size2;

    // Выделение памяти для результата XOR
    char *result = malloc(max_size);
    if (result == NULL) {
        perror("Ошибка выделения памяти для результата");
        exit(1);
    }

    // Выполнение побайтовой операции XOR
    for (size_t i = 0; i < size1; i++) {
        result[i] = data1[i%size1] ^ data2[i%size2];
    }

    // Сохранение результата в файл
    write_result(output_file, result, size1);

    printf("Обработано %zu байт. Результат сохранен в %s\n", max_size, output_file);

    // Освобождение выделенной памяти
    free(data1);
    free(data2);
    free(result);

    // Ожидание завершения дочерних процессов
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // Удаление именованных каналов из файловой системы
    unlink(FIFO1);
    unlink(FIFO2);
    unlink(RESULT_FIFO);

    return 0;
}
