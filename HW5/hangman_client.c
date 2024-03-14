#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 1024 
#define SERVER_IP "127.0.0.1"
#define PORT 8080

int client_fd;
struct sockaddr_in serv_addr;

void start_game(int sockfd);
void send_message(int sockfd, char *message, size_t msg_length);
void receive_and_print_server_response(int sockfd);
void clear_input_buffer();


int main() {
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

    // printf("Connected to server\n");
    start_game(client_fd);
    close(client_fd);
    return 0;
}

void start_game(int sockfd) {
    char guess[3];

    printf(">>>Ready to start game? (y/n): ");
    if (!fgets(guess, sizeof(guess), stdin)) {
        // printf("\nEOF received. Exiting game.\n");
        close(sockfd);
        exit(EXIT_SUCCESS);
    }
    // fgets(guess, sizeof(guess), stdin);

    if (tolower(guess[0]) == 'y') {
        // Send game start signal
        send_message(sockfd, "0", 1);
        // printf("Sent game start signal to server.\n"); // Debug message
        receive_and_print_server_response(sockfd);
        while (1) {
            printf(">>>Letter to guess: ");
            if (!fgets(guess, sizeof(guess), stdin)) {
                // printf("\nEOF received. Exiting game.\n");
                close(sockfd);
                exit(EXIT_SUCCESS);
            }
            if(strchr(guess, '\n') == NULL) {
                clear_input_buffer();
            }

            // printf("Guess is long: %d\n", strlen(guess));
            // guess[strcspn(guess, "\n")] = 0; // Remove trailing newline character
            //if the guess is more than one character, print an error message and prompt the user to guess again
            // printf("Guess is NOW only long: %d\n", strlen(guess));
            // fgets(guess, sizeof(guess), stdin);
            // clear_input_buffer();
            if (isalpha(guess[0]) && guess[1] == '\n') {
                guess[0] = tolower(guess[0]); // Normalize input to lowercase
                send_message(sockfd, guess, 1); // Send the guess to the server
                receive_and_print_server_response(sockfd); // Handle server response
            } else {
                printf("Error! Please guess one letter.\n");
            }
        }
    } else {
        printf("Exiting game.\n");
        close(client_fd);
        exit(EXIT_SUCCESS);
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
    // printf("bytes_received: %d\n", bytes_received);
    // printf("buffer: %s\n", buffer);
    if (bytes_received < 0) {
        printf("Nothing received from server.\n");
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }
    if (bytes_received > 0) { 
        buffer[bytes_received] = '\0'; // Null-terminate the data for safety
        // printf("msg_flag: %d\n", msg_flag);
        if (strncmp(buffer + 1, "The word was", strlen("The word was")) == 0) {
            printf("%s\n", buffer + 1);
            // printf("game should end here");
            close(sockfd);
            exit(EXIT_SUCCESS);
            // return;
        }
        unsigned char msg_flag = buffer[0];
        if (msg_flag == 0) {
            // It's a game control packet
            unsigned char word_length = buffer[1];
            unsigned char num_incorrect = buffer[2];

            // Print the display word on one line
            printf(">>>");
            //iterate through the buffer and print the display word with a space after each character, except for the last character
            for(int i = 0; i < word_length-1; i++) {
                printf("%c ", buffer[3 + i]);
            } 
            printf("%c\n", buffer[3 + word_length - 1]);

            // Print the incorrect guesses on a new line if there are any
            if (num_incorrect > 0) {
                printf(">>>Incorrect Guesses:");
                for (int i = 0; i < num_incorrect; i++) {
                    printf(" %c", buffer[3 + word_length + i]);
                }
                printf("\n");
            }

        } else {
            // It's a message
            // Ensure the message is null-terminated
            buffer[msg_flag + 1] = '\0';
            printf("%s\n", buffer + 1); // Print the message starting at the second byte
            if (strcmp((char *)(buffer + 1), "Game Over!") == 0 ||
                strcmp((char *)(buffer + 1), "You Win!") == 0 ||
                strcmp((char *)(buffer + 1), "You Lose :(") == 0) {
                // Clean up and exit
                close(sockfd);
                exit(EXIT_SUCCESS);
            }
        }
    }
}
