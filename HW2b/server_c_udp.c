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
            strcpy(response, "Sorry cannot compute!");
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
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to the specified address and port
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }
    while(1) {
        // Receive data from client
        int len = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &addr_len);
        buffer[len] = '\0';

        // printf("Connected to client on Port: %d", PORT);

        char response[BUFFER_SIZE];
        int result = process_string(buffer, response);
            // Send response to client
            sendto(sockfd, response, strlen(response), 0, (const struct sockaddr *)&client_addr, addr_len);
        if (result >= 0 && result < 10) {
            break;
        }
    }

    // Close the socket
    close(sockfd);

    return 0;
}
