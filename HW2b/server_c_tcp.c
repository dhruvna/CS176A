#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int process_string(const char *str, char *response) {
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
    int sockfd;
    char buffer[BUFFER_SIZE];

    int new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *hello = "Hello from server";

    // Create socket 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    // while(1) {
    //     // Receive data from client
    //     int len = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &addr_len);
    //     buffer[len] = '\0';

    //     // printf("Connected to client on Port: %d", PORT);

    //     char response[BUFFER_SIZE];
    //     int result = process_string(buffer, response);
    //         // Send response to client
    //         sendto(sockfd, response, strlen(response), 0, (const struct sockaddr *)&client_addr, addr_len);
    //     if (result >= 0 && result < 10) {
    //         break;
    //     }
    // }
    // Read data from the client
    valread = read(new_socket, buffer, BUFFER_SIZE);
    printf("Client: %s\n", buffer);

    // Send a response to the client
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    return 0;
}
