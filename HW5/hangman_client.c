#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 129 //128 is the max string length, include 1 more to null terminate it

int main(int argc, char *argv[]) { //take in server ip and port
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
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) { //bind to specific ip and port
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Get input from user
    printf("Ready to start game? (y/n):"); 
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Remove newline character or it will mess with server processing
    printf("Sending: %s\n", buffer);

    // Send data to server
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send data to server");
        exit(EXIT_FAILURE);
    }
    
    int bytesReceived;
    // Read data from the server
    while ((bytesReceived = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytesReceived] = '\0'; // Null-terminate the received data
        
        char *ptr = buffer; // Pointer to track the current position in the buffer
        char *end; // Pointer to track the end of the current message
        while ((end = strchr(ptr, '\n')) != NULL) {
            *end = '\0'; // Replace the newline with a null terminator to create a string for the current message
            if (*ptr) { // If the string is not empty
                printf("From server: %s\n", ptr); // Print the message
            }
            ptr = end + 1; // Move past the current message
        }

        if (*ptr) { // If there's remaining text after the last newline, print it
            printf("From server: %s\n", ptr);
        }
    }

    if (bytesReceived < 0) {
        perror("recv failed");
    } else if (bytesReceived == 0) {
        // printf("Server closed the connection.\n");
    }
    // Close the socket
    close(sockfd);
    return 0;
}
