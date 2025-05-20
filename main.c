#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUF_SIZE 1024
#define MAX_USERNAME 32

void *receive_messages(void *arg);

int main() {
    char username[MAX_USERNAME];
    int sender_fd, receiver_fd;
    struct sockaddr_in broadcast_addr;
    pthread_t recv_thread;

    // Get username
    printf("Enter your username: ");
    fgets(username, MAX_USERNAME, stdin);
    username[strcspn(username, "\n")] = '\0';

    // Setup receiver socket
    if ((receiver_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Receiver socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allow multiple binds
    int reuse = 1;
    if (setsockopt(receiver_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) {
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

    // Setup sender socket
    if ((sender_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Sender socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Enable broadcast
    int broadcast_enable = 1;
    if (setsockopt(sender_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable))) {
        perror("Sender setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure broadcast address
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    // Start receive thread
    pthread_create(&recv_thread, NULL, receive_messages, (void *)&receiver_fd);

    printf("Start typing messages...\n");

    // Message input loop
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
    int sockfd = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUF_SIZE];

    while (1) {
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0,
                        (struct sockaddr *)&client_addr, &addr_len);
        if (n > 0) {
            buffer[n] = '\0';
            printf("%s\n", buffer);
        }
    }

    return NULL;
}
