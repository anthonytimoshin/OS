#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

// Глобальные переменные - ОДНИ на оба sender'а
volatile sig_atomic_t bit_ready = 0;
volatile sig_atomic_t current_bit = 0;
volatile sig_atomic_t ack_sent = 0;
volatile pid_t expected_sender = 0;

// Буферы
unsigned char buffer1[BUFFER_SIZE];
unsigned char buffer2[BUFFER_SIZE];
int buffer1_size = 0;
int buffer2_size = 0;

// Простой обработчик - только отмечает, что бит получен
void bit_handler(int sig) {
    current_bit = (sig == SIGUSR2); // SIGUSR2=1, SIGUSR1=0
    bit_ready = 1;
}

// Отдельный обработчик для ACK (не используется в получателе)
void ack_handler(int sig) {
    // Пустой - просто принимаем ACK
}

// Функция получения одного байта
unsigned char receive_byte_simple() {
    unsigned char byte = 0;
    
    for (int i = 0; i < 8; i++) {
        bit_ready = 0;
        
        // Ждём бит (без ACK - упрощаем!)
        int timeout = 0;
        while (!bit_ready && timeout < 1000) {
            usleep(100);
            timeout++;
        }
        
        if (bit_ready) {
            byte = (byte << 1) | current_bit;
        } else {
            printf("T");
            return 0; // Таймаут
        }
    }
    
    return byte;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <file1> <file2> <output>\n", argv[0]);
        return 1;
    }
    
    printf("=== XOR via Signals (Working Version) ===\n");
    printf("Main PID: %d\n\n", getpid());
    
    // Устанавливаем ОДИН обработчик для битов
    signal(SIGUSR1, bit_handler);
    signal(SIGUSR2, bit_handler);
    
    // === Получаем файл 1 ===
    printf("--- Receiving file 1: %s ---\n", argv[1]);
    
    // Создаём pipe для синхронизации
    int sync_pipe1[2];
    pipe(sync_pipe1);
    
    pid_t sender1 = fork();
    if (sender1 == 0) {
        close(sync_pipe1[0]); // закрываем чтение
        
        char pid_str[16];
        sprintf(pid_str, "%d", getppid());
        
        // Записываем свой PID в pipe
        pid_t mypid = getpid();
        write(sync_pipe1[1], &mypid, sizeof(pid_t));
        close(sync_pipe1[1]);
        
        // Запускаем sender
        execl("./sender_fixed", "sender_fixed", argv[1], pid_str, NULL);
        perror("execl failed");
        exit(1);
    }
    
    close(sync_pipe1[1]);
    
    // Читаем PID sender'а
    pid_t sender1_pid;
    read(sync_pipe1[0], &sender1_pid, sizeof(pid_t));
    close(sync_pipe1[0]);
    
    printf("Sender1 PID: %d\n", sender1_pid);
    
    // Получаем данные
    int count1 = 0;
    while (count1 < BUFFER_SIZE) {
        unsigned char byte = receive_byte_simple();
        
        if (byte == 0 && count1 > 100) { // Предполагаем конец
            break;
        }
        
        buffer1[count1++] = byte;
        
        if (count1 % 20 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    
    buffer1_size = count1;
    printf("\nReceived %d bytes from file 1\n", buffer1_size);
    
    // Ждём завершения sender1
    int status1;
    waitpid(sender1, &status1, 0);
    
    // === Получаем файл 2 ===
    printf("\n--- Receiving file 2: %s ---\n", argv[2]);
    
    int sync_pipe2[2];
    pipe(sync_pipe2);
    
    pid_t sender2 = fork();
    if (sender2 == 0) {
        close(sync_pipe2[0]);
        
        char pid_str[16];
        sprintf(pid_str, "%d", getppid());
        
        pid_t mypid = getpid();
        write(sync_pipe2[1], &mypid, sizeof(pid_t));
        close(sync_pipe2[1]);
        
        execl("./sender_fixed", "sender_fixed", argv[2], pid_str, NULL);
        perror("execl failed");
        exit(1);
    }
    
    close(sync_pipe2[1]);
    
    pid_t sender2_pid;
    read(sync_pipe2[0], &sender2_pid, sizeof(pid_t));
    close(sync_pipe2[0]);
    
    printf("Sender2 PID: %d\n", sender2_pid);
    
    // Получаем данные
    int count2 = 0;
    while (count2 < BUFFER_SIZE) {
        unsigned char byte = receive_byte_simple();
        
        if (byte == 0 && count2 > 100) {
            break;
        }
        
        buffer2[count2++] = byte;
        
        if (count2 % 20 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    
    buffer2_size = count2;
    printf("\nReceived %d bytes from file 2\n", buffer2_size);
    
    waitpid(sender2, NULL, 0);
    
    // === Выполняем XOR ===
    printf("\n--- Performing XOR ---\n");
    
    int xor_size = (buffer1_size < buffer2_size) ? buffer1_size : buffer2_size;
    
    if (xor_size == 0) {
        printf("Error: No data to XOR\n");
        return 1;
    }
    
    FILE *out = fopen(argv[3], "wb");
    if (!out) {
        perror("fopen output");
        return 1;
    }
    
    printf("XORing %d bytes...\n", xor_size);
    
    for (int i = 0; i < xor_size; i++) {
        unsigned char result = buffer1[i] ^ buffer2[i];
        fputc(result, out);
        
        if (i < 3) {
            printf("  %02x ^ %02x = %02x\n", buffer1[i], buffer2[i], result);
        }
    }
    
    fclose(out);
    
    printf("\n=== Complete ===\n");
    printf("Output: %s (%d bytes)\n", argv[3], xor_size);
    printf("\nTo decrypt: %s %s %s decrypted.txt\n", 
           argv[0], argv[3], argv[2]);
    
    return 0;
}
