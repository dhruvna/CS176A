#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>


#define BUFFER_SIZE 1024 

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
            fclose(file);
            return word;
        }
        current_word++;
    }

    fclose(file);
    return NULL; // Error: random line not found
}

int main(int argc, char *argv[]) { // take in port number
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // char* word = getWord();
    // printf("Word: %s", word);
    int PORT = atoi(argv[1]);
    int sockfd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind socket to the specified port
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        char buffer[BUFFER_SIZE];
        char response[BUFFER_SIZE];
        char tempBuffer[BUFFER_SIZE];

        // Accept client connection
        if ((new_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Receive message from client
        int len = recv(new_socket, buffer, BUFFER_SIZE, 0);
        printf("Received: %s\n", buffer);

        // Close the socket for this client
        close(new_socket);
    }
    close(sockfd);
    return 0;
}

// Write a fucntion to Read in the file hangman_words.txt and randomly select one line from it