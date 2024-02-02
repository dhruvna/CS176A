#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
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

    // Set server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Get input from user
    printf("Enter string: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Remove newline character or it will mess with server processing

    // Send data to server
    const char* message = "Hello, server!";
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Failed to send data to server");
        exit(EXIT_FAILURE);
    }

    // Receive response from server
    socklen_t serverAddrLen = sizeof(serverAddr);
    while (1) {
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
        buffer[len] = '\0'; // Null-terminate the received string

        printf("From server: %s\n", buffer);

        // Check for end of communication conditions
        if (strcmp(buffer, "Sorry cannot compute!") == 0 || (len == 1 && buffer[0] >= '0' && buffer[0] <= '9')) {
            break;
        }
    }

    // Close socket
    close(sockfd);

    return 0;
}
