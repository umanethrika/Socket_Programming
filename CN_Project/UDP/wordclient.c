
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024    // Maximum buffer size for data transfer
#define SERVER_IP "127.0.0.1" // Server IP address
#define PORT 8080           // Server port

int main(int argc, char *argv[]) {
    int sockfd;                    // Socket file descriptor
    struct sockaddr_in server_addr; // Structure to store server address
    char buffer[MAX_BUFFER];        // Buffer to hold received data
    char word_request[MAX_BUFFER];  // Buffer to hold word request
    socklen_t server_len;           // Length of the server address
    FILE *output_file;              // File pointer for output file

    // Check if the user provided the correct number of arguments
    if (argc != 3) {
        printf("Usage: %s <input_filename> <output_filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Step 1: Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Clear the server address structure

    // Step 2: Configure server address
    server_addr.sin_family = AF_INET;             // Set address family to IPv4
    server_addr.sin_port = htons(PORT);           // Set server port
    if (inet_aton(SERVER_IP, &server_addr.sin_addr) <= 0) { // Convert IP address to binary
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    server_len = sizeof(server_addr); // Set the length of the server address

    // Step 3: Send the input filename to the server
    sendto(sockfd, argv[1], strlen(argv[1]), 0,
           (const struct sockaddr *)&server_addr, server_len);

    // Step 4: Receive the server's initial response
    memset(buffer, 0, MAX_BUFFER); // Clear the buffer
    recvfrom(sockfd, buffer, MAX_BUFFER, 0,
             (struct sockaddr *)&server_addr, &server_len);

    // Step 5: Check if the file was not found on the server
    if (strcmp(buffer, "NOTFOUND") == 0) {
        printf("FILE NOT FOUND\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 6: Check for the expected "HELLO" message
    if (strcmp(buffer, "HELLO") != 0) {
        printf("Invalid file format: First word is not HELLO\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 7: Open the output file for writing
    output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        perror("Cannot create output file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Write "HELLO" to the output file
    fprintf(output_file, "%s\n", buffer);

    // Step 8: Request words from the server until "FINISH" is received
    int word_count = 1; // Counter for word requests
    while (1) {
        // Prepare the word request
        sprintf(word_request, "WORD %d", word_count);
        sendto(sockfd, word_request, strlen(word_request), 0,
               (const struct sockaddr *)&server_addr, server_len);

        // Receive the word from the server
        memset(buffer, 0, MAX_BUFFER); // Clear the buffer
        recvfrom(sockfd, buffer, MAX_BUFFER, 0,
                 (struct sockaddr *)&server_addr, &server_len);

        // Write the word to the output file
        fprintf(output_file, "%s\n", buffer);

        // Check if the "FINISH" message is received
        if (strcmp(buffer, "FINISH") == 0) {
            break;
        }

        word_count++; // Increment the word request counter
    }

    // Step 9: Close the output file and the socket
    fclose(output_file);
    close(sockfd);

    printf("File transfer completed successfully\n");
    return 0;
}