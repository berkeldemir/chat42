#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8888
#define BUF_SIZE 1024
#define MAX_USERNAME 32

// ANSI renk kodları
#define COLOR_USER "\033[1;32m"
#define COLOR_RESET "\033[0m"

void *receive_messages(void *arg);

typedef struct {
    int sockfd;
    char username[MAX_USERNAME];
} ThreadData;

int main() {
    char username[MAX_USERNAME];
    int sender_fd, receiver_fd;
    struct sockaddr_in broadcast_addr;
    pthread_t recv_thread;
    ThreadData thread_data;

    // Terminali temizle
    printf("\033[2J\033[H");

    // Kullanıcı adını env'den al
    char *env_user = getenv("USER");
    if (env_user != NULL) {
        strncpy(username, env_user, MAX_USERNAME - 1);
    } else {
        strncpy(username, "user", MAX_USERNAME - 1);
    }
    username[MAX_USERNAME - 1] = '\0';

    // Soket kurulumu (önceki kodla aynı)
    if ((receiver_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Receiver socket failed");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(receiver_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Receiver setsockopt failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(receiver_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if ((sender_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Sender socket failed");
        exit(EXIT_FAILURE);
    }

    int broadcast_enable = 1;
    if (setsockopt(sender_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Sender setsockopt failed");
        exit(EXIT_FAILURE);
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // Thread verilerini hazırla
    thread_data.sockfd = receiver_fd;
    strncpy(thread_data.username, username, MAX_USERNAME);
    
    // Thread başlat
    pthread_create(&recv_thread, NULL, receive_messages, &thread_data);

    // Mesaj gönderme döngüsü
    while (1) {
        char message[BUF_SIZE];
        fgets(message, BUF_SIZE, stdin);
        message[strcspn(message, "\n")] = '\0';

        char full_msg[BUF_SIZE + MAX_USERNAME + 4];
        snprintf(full_msg, sizeof(full_msg), "[%s] %s", username, message);

        sendto(sender_fd, full_msg, strlen(full_msg), 0,
              (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
    }

    close(sender_fd);
    close(receiver_fd);
    return 0;
}

void *receive_messages(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int sockfd = data->sockfd;
    char *my_username = data->username;
    
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    while (1) {
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                        (struct sockaddr *)&client_addr, &addr_len);
        if (n > 0) {
            buffer[n] = '\0';
            
            // Mesajı parse et
            char *username_end = strchr(buffer, ']');
            if (username_end) {
                *username_end = '\0';
                char *msg_username = buffer + 1;
                char *message = username_end + 2;

                // Kullanıcı adını karşılaştır
                if (strcmp(msg_username, my_username) == 0) {
                    ;
		} else {
                    printf("\r\033[K" COLOR_USER "%s" COLOR_RESET ": %s\n", 
                          msg_username, message);
                }
            } else {
                printf("\r\033[K%s\n", buffer);
            }
            fflush(stdout);
        }
    }
    return NULL;
}
