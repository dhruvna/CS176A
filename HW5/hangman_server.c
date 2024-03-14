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
#define BUFFER_SIZE 1024 
#define MAX_ATTEMPTS 6
#define MAX_WORDS 15
#define WORDS_FILE "hangman_words.txt"
#define MAX_WORD_LENGTH 11 // Biggest word is 8 letters + null terminator

typedef struct {
    int sock;
    char word[MAX_WORD_LENGTH]; // 8 letter word + null terminator
    char display_word[MAX_WORD_LENGTH]; // 8 letter word + null terminator
    char incorrect_guesses[27];
    int attempts_left; // = MAX_ATTEMPTS - # incorrect guesses
    int num_incorrect; 
} GameState;

int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *client_socket);

void init_display_word(char *display, const char *word);
void process_guess(GameState *game, char guess);
void send_game_state(GameState *game);
void send_message(GameState *game, const char *message);

char *select_word(char *word);
void load_words(char words[][MAX_WORD_LENGTH], int *count);

int main() {
    
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
        // printf("Client connected successfully.\n"); // Add this line

        pthread_mutex_lock(&client_count_mutex);
        if (client_count >= MAX_CLIENTS) {
            pthread_mutex_unlock(&client_count_mutex);
            // printf("Extra client connection closed.\n");
            //use standard socket sending to send "server-overloaded"
            send(new_socket, "Server overloaded", 17, 0);
            close(new_socket);
            continue;
        }
        client_count++;
        pthread_mutex_unlock(&client_count_mutex);

        GameState *game = malloc(sizeof(GameState));
        game->sock = new_socket;
        select_word(game->word);
        init_display_word(game->display_word, game->word);
        game->attempts_left = MAX_ATTEMPTS;
        game->num_incorrect = 0;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, (void*)game) < 0) {
            perror("Failed to create thread");
            free(game);
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

    // Wait for the game start signal (empty message)
    message_length = recv(game->sock, buffer, BUFFER_SIZE - 1, 0);
    if (message_length <= 0) {
        // Handle error or closed connection by the client
        printf("Client did not send start signal or closed the connection.\n");
        pthread_exit(NULL); // Or other cleanup and exit logic
    }
    // printf("Received game start signal: '%s'\n", buffer); // Debug message
    send_game_state(game);
    // Send initial game state
    // send_game_state(game);
    // recv(game->sock, buffer, BUFFER_SIZE - 1, 0);
    // printf("Received: %s\n", buffer);
    // send_game_state(game); 

    // Receive and process guesses
    while((message_length = recv(game->sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[message_length] = '\0'; // Ensure the buffer is null-terminated
        process_guess(game, buffer[0]); // Process the first character as a guess
        if (strcmp(game->word, game->display_word) == 0) {
            //send a message saying "The word was " + game->word
            char gameWord[100];
            strcpy(gameWord, "The word was ");
            strcat(gameWord, game->word);
            //remove any white space after gameWord
            // gameWord[strcspn(gameWord, "\n")] = '\0'; // Remove newline character a diff way idk
            // gameWord[strcspn(gameWord, "\r")] = '\0';
// 
            // strcat(gameWord, "");
            send_message(game, gameWord);
            
            // printf("WORD: %s -----", gameWord);
            send_message(game, "You Win!\n");
            send_message(game, "Game Over!\n");
            break;
        } else if (game->attempts_left <= 0) {
            send_message(game, "You Lose :(\n");
            send_message(game, "Game Over!\n");
            break;
        } else {
            send_game_state(game);
        }
    }

    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    close(game->sock);
    free(game);
    pthread_exit(NULL);

}

void send_game_state(GameState *game) {
    unsigned char msg_flag = 0; // Indicates a game control packet
    unsigned char word_length = strlen(game->word);
    unsigned char num_incorrect = game->num_incorrect;

    char *packet = malloc(3 + word_length + num_incorrect);

    packet[0] = msg_flag;
    packet[1] = word_length;
    packet[2] = num_incorrect;
    // Add the display word (with guessed letters and underscores)
    for (int i = 0; i < word_length; ++i) {
        packet[3 + i] = game->display_word[i];
    }
    // Add incorrect guesses
    for(int i = 0; i < num_incorrect; i++) {
        packet[3 + word_length + i] = game->incorrect_guesses[i];
    }
    send(game->sock, packet, 3 + word_length + num_incorrect, 0);

    // Print the packet for debugging
    // for (int i = 0; i < (3 + word_length + num_incorrect); i++) {
    //     printf("%02X ", (unsigned char)packet[i]);
    // }
    // printf("\n");
    free(packet);
}

void send_message(GameState *game, const char *message) {
    unsigned char msg_flag = strlen(message); // Message length
    char packet[BUFFER_SIZE];
    packet[0] = msg_flag; // Set message flag to message length
    strcpy(packet + 1, message); // Copy the message
    send(game->sock, packet, 1 + msg_flag, 0);
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
    if (!match_found && strchr(game->incorrect_guesses, guess) == NULL) {
        game->incorrect_guesses[game->num_incorrect++] = guess;
        game->incorrect_guesses[game->num_incorrect] = '\0';
        game->attempts_left--;
    }
}



void init_display_word(char *display, const char *word) {
    while (*word != '\0') {
        *display++ = (*word++ == ' ') ? ' ' : '_';
    }
    *display = '\0';
}

char *select_word(char *word) {
    static char words[15][MAX_WORD_LENGTH];
    static int words_loaded = 0;
    if (!words_loaded) {
        int count = 0;
        load_words(words, &count);
        words_loaded = 1;
    }
    
    int index = rand() % 15; // Assuming you have exactly 15 words
    strcpy(word, words[index]);
    // printf("Selected word: %s\n", word);
    return word;
}

void load_words(char words[][MAX_WORD_LENGTH], int *count) {
    FILE *file = fopen(WORDS_FILE, "r");
    char line[MAX_WORD_LENGTH];

    if (!file) {
        perror("Unable to open words file");
        exit(EXIT_FAILURE);
    }
    
    while (fgets(line, sizeof(line), file) && *count < MAX_WORDS) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline character a diff way idk
        line[strcspn(line, "\r")] = '\0'; // Remove carriage return character same thing
        printf("Loaded word: %s", line);
        printf("\n");
        strcpy(words[(*count)++], line);
    }
    fclose(file);
}
