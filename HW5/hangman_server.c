#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024 

int client_count = 0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *client_socket);
int setupServer();
char *getWord();

void *handle_client(void *client_socket) {
    int sock = *((int *)client_socket);
    free(client_socket);

    pthread_mutex_lock(&client_count_mutex);
    if (client_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&client_count_mutex);
        const char *message = "server-overloaded";
        printf("Extra client connection closed.\n");
        send(sock, message, strlen(message), 0);
        close(sock);
        return NULL;
    }
    client_count++;
    pthread_mutex_unlock(&client_count_mutex);

    // Handle client communication here
    char buffer[1024];
    while (recv(sock, buffer, 1024, 0) > 0) {
        printf("Message from client: %s\n", buffer);
        send(sock, buffer, strlen(buffer), 0); // Echo back the received message
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    }

    pthread_mutex_lock(&client_count_mutex);
    client_count--;
    pthread_mutex_unlock(&client_count_mutex);

    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) { // take in port number
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int PORT = atoi(argv[1]);

    int server_fd, new_socket, *new_sock;
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
    if (listen(server_fd, 3) < 0) {
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

        new_sock = malloc(1);
        *new_sock = new_socket;
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            perror("could not create thread");
            continue;
        }

        // printf("Client connected\n");
        // char* word = getWord();
        // printf("Word: %s\n", word);

        // // Receive message from client
        // int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
        // printf("Received: %s\n", buffer);
        // // if the received buffer is "y", then start the game
        // // Close the socket for this client

        
        // close(new_socket);
    }
    close(server_fd);
    return 0;
}

char* getWord() {
    FILE *file = fopen("hangman_words.txt","r");
    if(file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    int num_words = 0;
    char c;
    while ((c = fgetc(file)) != EOF) {
        if(c == '\n') {
            num_words++;
        }
    }
    rewind(file);

    srand(time(NULL));
    int random_word = rand() % num_words;

    char* word = NULL;
    size_t len = 0;
    ssize_t read;
    int current_word = 0;
    while ((read = getline(&word, &len, file)) != -1) {
        if (current_word == random_word) {
            size_t ln = strlen(word) - 1;
            if (word[ln] == '\n')
                word[ln] = '\0';
            fclose(file);
            return word;
        }
        current_word++;
    }

    fclose(file);
    return NULL; // Error: random line not found
}  