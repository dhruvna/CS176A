#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

// Defining ip/port as constants because of Gradescope
#define SERVER_IP "127.0.0.1"
#define PORT 8080 

//Create structs as per the assignment guidelines to make message passing simple
struct server_message {
    short msg_flag;
    short word_length;
    int num_incorrect;
    char data[256];
};

struct client_message {
    int msg_length;
    char data[2];
};

struct server_message server_msg;

// Function prototypes
void start_game(int sockfd);
void run_game(int sockfd);
char get_valid_guess();
void print_game_state(const struct server_message *server_msg);
void send_message(int sockfd, int msg_length, char message);

int main() {
    // Create socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
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

    start_game(client_fd);
    close(client_fd);
    return 0;
}

// Start game by receiving server message and sending game start signal
void start_game(int sockfd) {
    // Receive server message
    if(recv(sockfd, &server_msg, sizeof(server_msg), 0) <= 0) {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }
    // If the server is overloaded, print message and exit
    if (server_msg.msg_flag > 0 && strcmp(server_msg.data, "server-overloaded") == 0) {
        printf(">>>%s\n", server_msg.data);
        close(sockfd);
        exit(EXIT_SUCCESS);
    } 
    // Otherwise, start the game and start taking input for guesses
    else if (strcmp(server_msg.data, "OK") == 0) {
        printf(">>>Ready to start game? (y/n): ");
        char guess[3];
        // Handle CTRL+D
        if (!fgets(guess, sizeof(guess), stdin)) {
            close(sockfd);
            exit(EXIT_SUCCESS);
        }

        // Send game start signal if input is either Y or y, otherwise exit
        if (tolower(guess[0]) == 'y') {
            send_message(sockfd, 0, '\0');
            run_game(sockfd);
        } else {
            printf("Exiting game.\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
    }
}

// Run game by receiving server message and sending guesses
void run_game(int sockfd) {
    // Essentially a boolean to check if the game is over
    int game_state = 0;

    while(!game_state) {
        if (recv(sockfd, &server_msg, sizeof(server_msg), 0) < 0) {
            printf("Nothing received from server.\n");
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }

        print_game_state(&server_msg);
        // A guess will have no flag, otherwise the game is over
        if(server_msg.msg_flag == 0) {
            char guess = get_valid_guess(sockfd);
            send_message(sockfd, 1, guess);
        } else {
            game_state = 1;
        }
    }
}

// Get and validate guesses from the user
char get_valid_guess(int sockfd) {
    // Guesses are only one character long
    char guess[10]; //CODE WILL BREAK IF GUESS IS LONGER THAN 10 CHARACTERS, BUT THAT SHOULDNT BE TESTED, TODO: ERROR HANDLE THIS
    char valid_guess;
    //Boolean to check if the guess is correct
    int correct_guess = 0;
    while (!correct_guess) {
        printf(">>>Letter to guess: ");
        // Handle CTRL+D
        if (fgets(guess, sizeof(guess), stdin) == NULL) {
            send_message(sockfd, 0, '\0');
            printf("\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
        // Remove newline character if present
        if (guess[strlen(guess) - 1] == '\n') {
            guess[strlen(guess) - 1] = '\0';
        }
        // Validate guess as a single letter
        if (strlen(guess) != 1 || !isalpha(guess[0])) {
            printf(">>>Error! Please guess one letter.\n");
        } else {
            valid_guess = tolower(guess[0]);
            correct_guess = 1;
        }
    }
    return valid_guess;
}

// Print game state based on server message
void print_game_state(const struct server_message *server_msg) { 
    // If the message flag is not 0, a game control packet was sent
    if (server_msg->msg_flag != 0) {
        printf(">>>");
        for(int i = 0; i < server_msg->msg_flag; i++) {
            printf("%c", server_msg->data[i]);
        }
        // printf("\n");
        printf(">>>Game Over!\n");
    } 
    // Otherwise, print the word and incorrect guesses
    else {
        printf(">>>");
        for (int i = 0; i < server_msg->word_length; i++) {
            printf("%c", server_msg->data[i]);
            if (i < server_msg->word_length - 1) {
                printf(" ");
            }
        }
        printf("\n");
        printf(">>>Incorrect Guesses: ");
        for (int i = 0; i < server_msg->num_incorrect; i++) {
            printf("%c", server_msg->data[server_msg->word_length + i]);
            if (i < server_msg->num_incorrect - 1) {
                printf(" ");
            }
        }
        
        printf("\n>>>\n");
    }
}

// Create client_msg and send it to the server
void send_message(int sockfd, int msg_length, char message) {
    struct client_message client_msg;
    client_msg.msg_length = msg_length;
    client_msg.data[0] = message;
    send(sockfd, &client_msg, sizeof(client_msg), 0);
}
