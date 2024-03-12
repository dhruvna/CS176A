#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 129 //128 is the max string length, include 1 more to null terminate it

char buffer[BUFFER_SIZE];
int client_fd;
struct sockaddr_in serv_addr;

void initializeGame();

int main(int argc, char *argv[]) { //take in server ip and port
    if(argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    const char* SERVER_IP = argv[1];
    int PORT = atoi(argv[2]);
    // Set server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) { //bind to specific ip and port
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Initialize game
    // initializeGame();

    // // Get input from user
    // printf("Ready to start game? (y/n):"); 
    // fgets(buffer, BUFFER_SIZE, stdin);
    // if(buffer[0] == 'n') {
    //     // printf("Goodbye\n");
    //     close(client_fd);
    //     exit(EXIT_SUCCESS);
    // } else if(buffer[0] != 'y') {
    //     printf("Invalid input. Please enter 'y' or 'n'\n");
    //     exit(EXIT_FAILURE);
    // } else {
    //     buffer[strcspn(buffer, "\n")] = 0; // Remove newline character or it will mess with server processing
    //     printf("Sending: %s\n", buffer);     
    // }

    // // Send data to server
    // if (send(client_fd, buffer, strlen(buffer), 0) < 0) {
    //     perror("Failed to send data to server");
    //     exit(EXIT_FAILURE);
    // }
    
    int bytesReceived;
    // Read data from the server
    while ((bytesReceived = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0) {
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
    close(client_fd);
    return 0;
}

void initializeGame() {
    // Get input from user
    printf("Ready to start game? (y/n):"); 
    fgets(buffer, BUFFER_SIZE, stdin);
    printf("buffer: %s\n", buffer);
    if(buffer[0] == 'n') {
        // printf("Goodbye\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
    } else if(buffer[0] != 'y') {
        printf("Invalid input. Please enter 'y' or 'n'\n");
        exit(EXIT_FAILURE);
    } else {
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character or it will mess with server processing
        printf("Sending: %s\n", buffer);     
    }

    // Send data to server
    if (send(client_fd, buffer, strlen(buffer), 0) < 0) {
        perror("Failed to send data to server");
        exit(EXIT_FAILURE);
    }
}

