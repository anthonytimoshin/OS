#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// ОЧЕНЬ ПРОСТОЙ sender - без ожидания ACK

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file> <receiver_pid>\n", argv[0]);
        return 1;
    }
    
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("fopen");
        return 1;
    }
    
    pid_t receiver = atoi(argv[2]);
    
    printf("Sender %d: Sending %s to %d\n", 
           getpid(), argv[1], receiver);
    
    int byte_count = 0;
    int c;
    
    // Читаем и отправляем с небольшой задержкой
    while ((c = fgetc(file)) != EOF) {
        unsigned char byte = (unsigned char)c;
        
        // Отправляем 8 битов
        for (int i = 7; i >= 0; i--) {
            int bit = (byte >> i) & 1;
            
            if (bit == 0) {
                kill(receiver, SIGUSR1);
            } else {
                kill(receiver, SIGUSR2);
            }
            
            // Небольшая задержка между битами
            usleep(1000); // 1ms
        }
        
        byte_count++;
        
        if (byte_count % 10 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    
    // Отправляем 8 нулей как маркер конца
    for (int i = 0; i < 8; i++) {
        kill(receiver, SIGUSR1);
        usleep(1000);
    }
    
    fclose(file);
    
    printf("\nSender %d finished (%d bytes)\n", getpid(), byte_count);
    
    return 0;
}
