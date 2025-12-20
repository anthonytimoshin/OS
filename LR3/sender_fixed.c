#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // Проверка количества аргументов командной строки
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <input_file> <output_fifo>\n", argv[0]);
        exit(1);
    }

    const char *input_file = argv[1];
    const char *output_fifo = argv[2];

    // Открытие входного файла в режиме только для чтения
    int input_fd = open(input_file, O_RDONLY);
    if (input_fd == -1) {
        perror("Ошибка открытия входного файла");
        exit(1);
    }

    // Открытие именованного канала (FIFO) для записи
    // Блокируется до тех пор, пока другой процесс не откроет канал для чтения
    int output_fd = open(output_fifo, O_WRONLY);
    if (output_fd == -1) {
        perror("Ошибка открытия именованного канала");
        close(input_fd);
        exit(1);
    }

    // Определение размера файла
    // Перемещение указателя в конец файла и получение позиции
    off_t file_size = lseek(input_fd, 0, SEEK_END);
    lseek(input_fd, 0, SEEK_SET);  // Возврат указателя в начало файла

    // Отправка размера файла через канал
    // Получатель сначала читает размер, чтобы знать, сколько данных ожидать
    if (write(output_fd, &file_size, sizeof(file_size)) != sizeof(file_size)) {
        perror("Ошибка записи размера файла");
        close(input_fd);
        close(output_fd);
        exit(1);
    }

    // Чтение файла блоками и отправка через канал
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
        // Запись прочитанного блока в канал
        if (write(output_fd, buffer, bytes_read) != bytes_read) {
            perror("Ошибка записи в канал");
            break;
        }
    }

    if (bytes_read == -1) {
        perror("Ошибка чтения файла");
    }

    // Закрытие файловых дескрипторов
    close(input_fd);
    close(output_fd);

    return 0;
}
