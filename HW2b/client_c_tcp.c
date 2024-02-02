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
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Connection established, do something...
    // Get input from user
    printf("Enter string: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Remove newline character or it will mess with server processing
    // printf("Sending: %s\n", buffer);

    // Send data to server
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send data to server");
        exit(EXIT_FAILURE);
    }
    
    // Receive response from server
    while (1) {
        int len = recv(sockfd, buffer, sizeof(buffer), 0);
            if (len <= 0) {
                if (len == 0) {
                    // Server closed the connection
                    // printf("Server closed the connection.\n");
                } else {
                    // An error occurred
                    perror("Failed to receive data from server");
                }
                break; // Exit loop
            }
        buffer[len] = '\0'; // Null-terminate the received string
        printf("From server: %s\n", buffer);
    }

    // Close the socket
    close(sockfd);
    return 0;
}
