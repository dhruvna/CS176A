#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 129 //128 is the max string length, include 1 more to null terminate it

int sum_of_digits(const char *str, char *response) { //take in string input, create a string response
    int sum = 0;
    while (*str) { //iterate
        if (*str < '0' || *str > '9') { //compare to ascii values, if outside of range it is not a number
            strcpy(response, "Sorry, cannot compute!");
            return -1;
        }
        sum += *str - '0';
        str++;
    }
    sprintf(response, "%d", sum); //convert sum to string for sending via socket
    return sum;
}

int main(int argc, char *argv[]) { // take in port number
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int PORT = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    //configure socket
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to the specified port 
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        char buffer[BUFFER_SIZE] = {0};
        char response[BUFFER_SIZE] = {0};
        char tempBuffer[BUFFER_SIZE] = {0}; // Temporary buffer for intermediate sums

        // Receive data from client
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &addr_len);
        buffer[len] = '\0'; // Null-terminate the received string

        // Calculate sum of digits
        int sum = sum_of_digits(buffer, response);

        // Send initial response or error message
        sendto(sockfd, response, strlen(response), 0, (const struct sockaddr *)&client_addr, addr_len);
       
        //for any subsequent sums if needed
        while (sum >= 10) {
            strcpy(tempBuffer, response); // Use the previous response as the next input
            sum = sum_of_digits(tempBuffer, response); // Calculate the new sum
            sendto(sockfd, response, strlen(response), 0, (const struct sockaddr *)&client_addr, addr_len); // Send intermediate sum
        }
    }
}
