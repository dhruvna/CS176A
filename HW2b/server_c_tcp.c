#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int sum_of_digits(const char *str, char *response) {
    int sum = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] < '0' || str[i] > '9') { //compare character with these ascii values essentially
            strcpy(response, "Sorry, cannot compute!");
            return -1; //if out of bounds return and respond accordingly
        }
        sum += str[i] - '0';
    }
    sprintf(response, "%d", sum); // Convert sum into string
    return sum; // Return sum to check if it's a single digit
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int PORT = atoi(argv[1]);
    int sockfd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the specified address and port
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        char buffer[BUFFER_SIZE];
        char response[BUFFER_SIZE];
        char tempBuffer[BUFFER_SIZE];

        // Accept client connection
        if ((new_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Receive message from client
        int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
        // printf("Received: %s\n", buffer);
        if (len > 0) {
            buffer[len] = '\0';

            int sum = sum_of_digits(buffer, response);
            char* str = malloc (sizeof(response) + 2);
            strcpy(str, response);
            strcat(str, "\n");
            // Send initial response or error message
            send(new_socket, response, strlen(response), 0);
            // printf("Sent: %s\n", response);
            // For any subsequent sums if needed
            while (sum >= 10) {
                strcpy(tempBuffer, response); // Use the previous response as the next input
                sum = sum_of_digits(tempBuffer, response); // Calculate the new sum
                char* str = malloc (sizeof(response) + 2);
                strcpy(str, response);
                strcat(str, "\n");
                send(new_socket, response, strlen(response), 0); // Send intermediate sum
            }
        }

        // Close the socket for this client
        close(new_socket);
    }
    close(sockfd);
    return 0;
}
