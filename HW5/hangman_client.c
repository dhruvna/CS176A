#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8085

struct server_message server_msg;

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

void send_message(int sockfd, int msg_length, char message);

void start_game(int sockfd);
void run_game(int sockfd);
char get_valid_guess();
void print_game_state(const struct server_message *server_msg);

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

void start_game(int sockfd) {
    if(recv(sockfd, &server_msg, sizeof(server_msg), 0) <= 0) {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }
    if (server_msg.msg_flag > 0 && strcmp(server_msg.data, "server-overloaded") == 0) {
        printf(">>>%s\n", server_msg.data);
        close(sockfd);
        exit(EXIT_SUCCESS);
    } else if (strcmp(server_msg.data, "OK") == 0) {
        printf(">>>Ready to start game? (y/n): ");
        char guess[3];
        if (!fgets(guess, sizeof(guess), stdin)) {
            // printf("\nEOF received. Exiting game.\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }

        if (tolower(guess[0]) == 'y') {
            // Send game start signal
            send_message(sockfd, 0, '\0');
            // printf("Sent game start signal to server.\n"); // Debug message
            run_game(sockfd);
        } else {
            printf("Exiting game.\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
    }
}

// void run_game(int sockfd) {
//     int game_state = 0;
//     char guess[100];

//     while(!game_state) {
//         if (recv(sockfd, &server_msg, sizeof(server_msg), 0) < 0) {
//             printf("Nothing received from server.\n");
//             perror("Receive failed");
//             exit(EXIT_FAILURE);
//         }
//         if(server_msg.msg_flag != 0) {
//             printf(">>>");
//             for(int i = 0; i < server_msg.msg_flag; i++) {
//                 printf("%c", server_msg.data[i]);
//             }
//             printf("\n");
//             printf(">>>GAME OVER!\n");
//             game_state = 1;
//         }
//         else if (server_msg.msg_flag == 0) {
//             printf(">>>");
//             for (int i = 0; i < server_msg.word_length; i++) {
//                 printf("%c", server_msg.data[i]);
//                 if (i < server_msg.word_length - 1) {
//                     printf(" ");
//                 }
//             }
//             printf("\n");
//             printf(">>>Incorrect Guesses:");
//             for (int i = 0; i < server_msg.num_incorrect; i++) {
//                 printf(" %c", server_msg.data[server_msg.word_length + i]);
//             }
//             printf("\n>>>\n");
//             int correct_guess = 0;
//             while (!correct_guess) {
//                 printf(">>>Letter to guess: ");
//                 if (fgets(guess, sizeof(guess), stdin) == NULL) {
//                     send_message(sockfd, 0, '\0');
//                     printf("\n");
//                     close(sockfd);
//                     exit(EXIT_SUCCESS);
//                 }
//                 if (guess[strlen(guess) - 1] == '\n') {
//                     guess[strlen(guess) - 1] = '\0';
//                 }
//                 if (strlen(guess) != 1 || !isalpha(guess[0])) {
//                     printf(">>>Error! Please guess one letter.\n");
//                     continue;
//                 } else {
//                     guess[0] = tolower(guess[0]);
//                     correct_guess = 1;
//                 }
//             }
//             send_message(sockfd, 1, guess[0]);
//         }
//     }
// }

void run_game(int sockfd) {
    int game_state = 0;

    while(!game_state) {
        if (recv(sockfd, &server_msg, sizeof(server_msg), 0) < 0) {
            printf("Nothing received from server.\n");
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }

        print_game_state(&server_msg);

        if(server_msg.msg_flag == 0) {
            char guess = get_valid_guess(sockfd);
            send_message(sockfd, 1, guess);
        } else {
            game_state = 1;
        }
    }
}

char get_valid_guess(int sockfd) {
    char guess[100];
    char valid_guess;
    int correct_guess = 0;
    while (!correct_guess) {
        printf(">>>Letter to guess: ");
        if (fgets(guess, sizeof(guess), stdin) == NULL) {
            send_message(sockfd, 0, '\0');
            printf("\n");
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
        if (guess[strlen(guess) - 1] == '\n') {
            guess[strlen(guess) - 1] = '\0';
        }
        if (strlen(guess) != 1 || !isalpha(guess[0])) {
            printf(">>>Error! Please guess one letter.\n");
        } else {
            valid_guess = tolower(guess[0]);
            correct_guess = 1;
        }
    }
    return valid_guess;
}

void print_game_state(const struct server_message *server_msg) { 
    if (server_msg->msg_flag != 0) {
        printf(">>>");
        for(int i = 0; i < server_msg->msg_flag; i++) {
            printf("%c", server_msg->data[i]);
        }
        printf("\n");
        printf(">>>GAME OVER!\n");
    } else {
        printf(">>>");
        for (int i = 0; i < server_msg->word_length; i++) {
            printf("%c", server_msg->data[i]);
            if (i < server_msg->word_length - 1) {
                printf(" ");
            }
        }
        printf("\n");
        printf(">>>Incorrect Guesses:");
        for (int i = 0; i < server_msg->num_incorrect; i++) {
            printf(" %c", server_msg->data[server_msg->word_length + i]);
        }
        printf("\n>>>\n");
    }
}

void send_message(int sockfd, int msg_length, char message) {
    struct client_message client_msg;
    client_msg.msg_length = msg_length;
    client_msg.data[0] = message;
    send(sockfd, &client_msg, sizeof(client_msg), 0);
}
