#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024 
#define MAX_ATTEMPTS 6
#define WORDS_FILE "hangman_words.txt"

typedef struct {
    int sock;
    char word[9]; // 8 letter word + null terminator
    char display_word[9]; // 8 letter word + null terminator
    int attempts_left;
} GameState;

int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *client_socket);
char *select_word();
void init_display_word(char *display, char *word);
void process_guess(GameState *game, char guess);

int main(int argc, char *argv[]) { // take in port number
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int PORT = atoi(argv[1]);

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
        printf("Client connected successfully.\n"); // Add this line

        pthread_mutex_lock(&client_count_mutex);
        if (client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&client_count_mutex);
            const char *message = "server-overloaded";
            printf("Extra client connection closed.\n");
            send(new_socket, message, strlen(message), 0);
            close(new_socket);
            continue;
        }
        client_count++;
        pthread_mutex_unlock(&client_count_mutex);

        GameState *game = malloc(sizeof(GameState));
        game->sock = new_socket;
        strcpy(game->word, select_word());
        init_display_word(game->display_word, game->word);
        game->attempts_left = MAX_ATTEMPTS;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void*)game) < 0) {
            perror("Failed to create thread");
            continue;
        }
    }
    close(server_fd);
    return 0;
}

void *handle_client(void *client_socket) {
    GameState *game = (GameState *)client_socket;
    char buffer[BUFFER_SIZE];
    ssize_t message_length;

    while((message_length = recv(game->sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[message_length] = '\0'; // Ensure the buffer is null-terminated
        process_guess(game, buffer[0]); // Process the first character as a guess

        if (strcmp(game->word, game->display_word) == 0) {
            send(game->sock, "You Won!!\n", 29, 0);
            break;
        } else if (game->attempts_left <= 0) {
            send(game->sock, "You lose!\n", 20, 0);
            break;
        } else {
            // Send current state of the game to the client
            char game_state[BUFFER_SIZE];
            snprintf(game_state, BUFFER_SIZE, "Word: %s Attempts left: %d\n", game->display_word, game->attempts_left);
            send(game->sock, game_state, strlen(game_state), 0);
        }
    }

    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    close(game->sock);
    free(game);
    return NULL;
    
}

char* select_word() {
    static char word[9]; // 8 letter word + null terminator
    FILE *file = fopen("hangman_words.txt","r");
    if(file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    int num_words = 0;
    while (fgets(word, sizeof(word), file) != NULL) {
        num_words++;
    }
    int chosen_word = rand() % num_words;
    fseek(file, 0, SEEK_SET);
    for (int i = 0; i <= chosen_word; i++) {
        fgets(word, sizeof(word), file);
    }
    word[strcspn(word, "\n")] = 0; // Remove newline
    fclose(file);
    return word;

    fclose(file);
    return NULL; // Error: random line not found
}  

void init_display_word(char *display, char *word) {
    while (*word != '\0') {
        *display++ = (*word++ == ' ') ? ' ' : '_';
    }
    *display = '\0';
}

void process_guess(GameState *game, char guess) {
    int match_found = 0;
    guess = tolower(guess);
    for (int i = 0; game->word[i] != '\0'; i++) {
        if(tolower(game->word[i]) == guess) { // Convert to lowercase for comparison
            if(game->display_word[i] == '_') { // Only update if not already guessed
                game->display_word[i] = game->word[i];
                match_found = 1;
            }
        }
    }
    if (!match_found) game->attempts_left--;
}