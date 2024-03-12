#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define SERVER_MSG_LEN 1
#define MAX_GUESS_LENGTH 2
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

    start_game(client_fd); // Start game after connection is established (no need to initialize game)

    close(client_fd);
    return 0;
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { } // Discard characters until end-of-line or end-of-file
}

void start_game(int sockfd) {
    char guess[MAX_GUESS_LENGTH];
    char start_signal[SERVER_MSG_LEN] = {0}; // Empty message to signal game start

    printf("Ready to start game? (y/n): ");
    
    fgets(guess, MAX_GUESS_LENGTH, stdin);
    clear_input_buffer();

    if (guess[0] == 'n') {
        printf("Exiting game.\n");
        exit(EXIT_SUCCESS);
    }

    // Send game start signal
    send_message(sockfd, start_signal, sizeof(start_signal));

    while (1) {
        printf("Letter to guess: ");
        fgets(guess, MAX_GUESS_LENGTH, stdin);
        clear_input_buffer();
        guess[strspn(guess, "\n")] = 0;
        if(strlen(guess) == 0) {
            // printf("Your Guess: %s\n", guess);
            // printf("Error! Please guess one letter.\n");
            continue;
        }
        
        // Validate input
        if(strlen(guess) > 1) {
            printf("Error! Please guess one letter.\n");
            // while(getchar() != '\n'); // Flush stdin
            continue;
        }

        // Convert to lowercase before sending to server
        guess[0] = tolower(guess[0]);
        send_message(sockfd, guess, 1); // Send only the first character
        receive_and_print_server_response(sockfd);
    }
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
