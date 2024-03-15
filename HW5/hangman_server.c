#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

#define PORT 8085
#define MAX_CLIENTS 3
#define MAX_WORDS 15
#define MAX_WORD_LENGTH 11 // Biggest word is 8 letters + null terminator

#define MAX_ATTEMPTS 6
#define WORDS_FILE "hangman_words.txt"

char words[MAX_WORDS][MAX_WORD_LENGTH];
int word_count = 0;

int client_count = 0;
pthread_mutex_t client_count_mutex;

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

void *handle_client(void *client_socket);

void send_message(int sockfd, const char *message, int msg_flag, int word_length, int num_incorrect);

char *select_word(char *word);
void load_words(char words[][MAX_WORD_LENGTH], int *word_count);

int main() {
    pthread_mutex_init(&client_count_mutex, NULL);

    load_words(words, &word_count);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    while(1) {
        // Accept client connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&client_count_mutex);
        if (client_count >= MAX_CLIENTS) {
            const char *overload_msg = "server-overloaded";
            printf("Extra client connection closed.\n");
            send_message(new_socket, overload_msg, strlen(overload_msg), 0, 0);
            close(new_socket);
        } else {
            send_message(new_socket, "OK", strlen("OK"), 0, 0);
            client_count++;
            int *temp_socket = malloc(sizeof(int));
            *temp_socket = new_socket;

            pthread_t tid;
            if (pthread_create(&tid, NULL, handle_client, (void*)temp_socket) < 0) {
                perror("Failed to create thread");
                free(temp_socket);
                continue;
            }
        }
        pthread_mutex_unlock(&client_count_mutex);
    }
    pthread_mutex_destroy(&client_count_mutex);
    close(server_fd);
    return 0;
}

void *handle_client(void *client_socket) {
    int new_socket = *(int*)client_socket;
    int index;

    struct server_message server_msg;
    struct client_message client_msg;
    // Wait for the game start signal (empty message)
    if (recv(new_socket, &client_msg, sizeof(client_msg), 0) <= 0) {
        // Handle error or closed connection by the client
        printf("Client did not send start signal or closed the connection.\n");
        pthread_mutex_lock(&client_count_mutex);
        client_count--;
        pthread_mutex_unlock(&client_count_mutex);

        close(new_socket);
        free(client_socket);
        return NULL;
    }

    if(client_msg.msg_length == 0) {
        if(word_count > 0) {
            index = rand() % MAX_WORDS;
            server_msg.msg_flag = 0;
            server_msg.word_length = strlen(words[index]);
            server_msg.num_incorrect = 0;
            memset(server_msg.data, 0, sizeof(server_msg.data));
            for (int i = 0; i < server_msg.word_length; i++) {
                server_msg.data[i] = '_';
            }
            send(new_socket, &server_msg, sizeof(server_msg), 0);
        } else {
            printf("No words received from hangman_words.txt\n");
        }
    }
    // Receive and process guesses
    int game_state = 0;
    while(!game_state) {
        if (recv(new_socket, &client_msg, sizeof(client_msg), 0) <= 0) {
            // Handle error or closed connection by the client
            printf("Client closed the connection / some error was thrown idk.\n");
            break;
        }
        printf("Received message: %s\n", client_msg.data);
        if (client_msg.msg_length == 1) {
            char guess = client_msg.data[0];
            
            int correct_guess = 0;
            for (int i = 0; i < server_msg.word_length; i++) {
                if (words[index][i] == guess) {
                    server_msg.data[i] = guess;
                    correct_guess = 1;
                }
            }
            if(correct_guess == 0) {
                server_msg.num_incorrect++;
                server_msg.data[server_msg.word_length + server_msg.num_incorrect - 1] = guess;
            }
            if(server_msg.num_incorrect >= 6) {
                memset(&server_msg.data, 0, sizeof(server_msg.data));
                strcpy(server_msg.data, "The word was: ");
                strcat(server_msg.data, words[index]);
                strcat(server_msg.data, "\n");
                strcat(server_msg.data, ">>>YOU LOSE!\n");
                server_msg.msg_flag = strlen(server_msg.data);
                server_msg.data[sizeof(server_msg.data) - 1] = '\0';
                server_msg.word_length = 0;
                server_msg.num_incorrect = 0;
                game_state = 1;
                send(new_socket, &server_msg, sizeof(server_msg), 0);
                break;
            }
            int word_complete = 1;
            for(int i = 0; i < server_msg.word_length; i++) {
                if(server_msg.data[i] == '_') {
                    word_complete = 0;
                    break;
                }
            }
            if(word_complete == 1) {
                memset(&server_msg.data, 0, sizeof(server_msg.data));
                strcpy(server_msg.data, "The word was: ");
                strcat(server_msg.data, words[index]);
                strcat(server_msg.data, "\n");
                strcat(server_msg.data, ">>>YOU WIN!\n");
                server_msg.msg_flag = strlen(server_msg.data);
                server_msg.data[sizeof(server_msg.data) - 1] = '\0';
                server_msg.word_length = 0;
                server_msg.num_incorrect = 0;
                game_state = 1;
                send(new_socket, &server_msg, sizeof(server_msg), 0);
                break;
            }
            server_msg.msg_flag = 0;
            send(new_socket, &server_msg, sizeof(server_msg), 0);
        }
        else if(client_msg.msg_length == 0) {
            printf("Client closed the connection.\n");
            pthread_mutex_lock(&client_count_mutex);
            client_count--;
            pthread_mutex_unlock(&client_count_mutex);

            close(new_socket);
            free(client_socket);
            pthread_exit(NULL);
        }
    }

    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    close(new_socket);
    free(client_socket);
    pthread_exit(NULL);

}

void send_message(int sockfd, const char *message, int msg_flag, int word_length, int num_incorrect) {
    struct server_message server_msg;
    memset(&server_msg, 0, sizeof(server_msg));
    server_msg.msg_flag = msg_flag;
    server_msg.word_length = word_length;
    server_msg.num_incorrect = num_incorrect;
    strcpy(server_msg.data, message);
    send(sockfd, &server_msg, sizeof(server_msg), 0);
};

void load_words(char words[][MAX_WORD_LENGTH], int *word_count) {
    char line[MAX_WORD_LENGTH];
    FILE *file = fopen(WORDS_FILE, "r");
    if (!file) {
        perror("Unable to open words file");
        exit(EXIT_FAILURE);
    }
    
    while (fgets(line, sizeof(line), file) && *word_count < MAX_WORDS) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline character a diff way idk
        line[strcspn(line, "\r")] = '\0'; // Remove carriage return character same thing
        printf("Loaded word: %s", line);
        printf("\n");
        strcpy(words[*word_count], line);
        (*word_count)++;
    }
    fclose(file);
}