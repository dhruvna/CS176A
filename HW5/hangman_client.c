#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 1024 

int client_fd;
struct sockaddr_in serv_addr;

void start_game(int sockfd);
void send_message(int sockfd, char *message, size_t msg_length);
void receive_and_print_server_response(int sockfd);
void clear_input_buffer();

int main(int argc, char *argv[]) { //take in server ip and port
    if(argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* SERVER_IP = argv[1];
    int PORT = atoi(argv[2]);
    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
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

    printf("Connected to server\n");
    start_game(client_fd);
    close(client_fd);
    return 0;
}

void start_game(int sockfd) {
    char guess[2];

    printf("Ready to start game? (y/n):");
    fgets(guess, sizeof(guess), stdin);
    clear_input_buffer();

    if (tolower(guess[0]) == 'n') {
        printf("Exiting game.\n");
        exit(EXIT_SUCCESS);
    }

    // Send game start signal
    send_message(sockfd, "", 0);

    while (1) {
        printf("Letter to guess: ");
        if (fgets(guess, sizeof(guess), stdin) == NULL) {
            continue; // Handle unexpected NULL input
        }
        clear_input_buffer();
        if (isalpha(guess[0])) {
            guess[0] = tolower(guess[0]); // Normalize input to lowercase
            send_message(sockfd, guess, 1); // Send the guess to the server
            receive_and_print_server_response(sockfd); // Handle server response
        } else {
            printf("Error! Please guess one letter.\n");
        }
    }
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void send_message(int sockfd, char *message, size_t msg_length) {
    if(send(sockfd, message, msg_length, 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
}

void receive_and_print_server_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the string
        printf("%s\n", buffer); // Display the message from the server
    }
}
