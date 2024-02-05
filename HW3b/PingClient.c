#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 1

int main(int argc, char *argv[]) { //take in server ip and port
    if(argc != 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* SERVER_IP = argv[1];
    int PORT = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    socklen_t serverAddrLen;
    //To keep track of packets sent and received and their rtt's
    int pack_recv = 0;
    double rtt[10];
    double min = rtt[1];
    double max = rtt[1];
    double sum = 0;

    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // send 10 messages, one every second. Messages should be of the form "PING {#} {timestamp}"
    for (int i = 1; i <= 10; i++) {
        
        struct timeval start_time, current_time;
        gettimeofday(&start_time, NULL);
        sprintf(buffer, "PING %d %ld", i, time(NULL));
        //Send message to server
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Failed to send data to server");
            exit(EXIT_FAILURE);
        }

        // Set timeout for receive operation
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT_SEC;
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
            perror("setsockopt");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        
        // Receive response
        int len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, &serverAddrLen);
        if (len < 0) {
            printf("Request timeout for seq#=%d\n", i);
            rtt[i] = -1;
        } else {
            // Process response
            pack_recv++;
            gettimeofday(&current_time, NULL);
            // Calculate RTT
            double elapsed_time = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
            rtt[i] = elapsed_time;
            if (rtt[i] < min) min = rtt[i];
            if (rtt[i] > max) max = rtt[i];
            sum += rtt[i];
            // Print response
            printf("PING received from %s: seq#=%d time=%.3lf ms\n", SERVER_IP, i, elapsed_time);
        }
        sleep(1);
    }
    //Final statistics
    printf("--- %s ping statistics ---\n", SERVER_IP);
    double avg = sum / 10;
    if(pack_recv == 0)
        printf("%d packets transmitted, %d received, %.0f%% packet loss\n", 10, pack_recv, (10 - pack_recv) / 10.0 * 100);
    else
        printf("%d packets transmitted, %d received, %.0f%% packet loss rtt min/avg/max = %.3lf %.3lf %.3lf ms\n", 10, pack_recv, (10 - pack_recv) / 10.0 * 100, min, avg, max);
    // Close socket
    close(sockfd);
    return 0;
}
