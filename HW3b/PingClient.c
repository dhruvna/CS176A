#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) { //take in server ip and port
    if(argc != 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* SERVER_IP = argv[1];
    int PORT = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }
    // send 10 messages, one every second. Messages should be of the form "PING {#} {timestamp}"
    for (int i = 1; i <= 10; i++) {
        sprintf(buffer, "PING %d %ld", i, time(NULL));
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Failed to send data to server");
            exit(EXIT_FAILURE);
        }
        //
        sleep(1);
    }

    socklen_t serverAddrLen = sizeof(serverAddr);
    while (1) {
        // Receive data from server
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
        buffer[len] = '\0'; // Null-terminate the received string

        printf("From server: %s\n", buffer);
    }

    // Close socket
    close(sockfd);
    return 0;
}
