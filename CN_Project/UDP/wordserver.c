

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024 // Maximum size of buffer for data transfer
#define PORT 8080       // Port number the server will listen on

int main() {
    int sockfd;                          // Socket file descriptor
    struct sockaddr_in server_addr, client_addr; // Structures for server and client addresses
    char buffer[MAX_BUFFER];            // Buffer for sending/receiving data
    socklen_t client_len = sizeof(client_addr); // Length of client address
    FILE *file;                         // File pointer to access requested files
    ssize_t recv_len, send_len;         // Variables to store send/receive data length

    // Step 1: Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Step 2: Clear and configure server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to all available interfaces
    server_addr.sin_port = htons(PORT);       // Convert port to network byte order

    // Step 3: Bind the socket to the server address
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Step 4: Receive the filename from the client
    memset(buffer, 0, MAX_BUFFER); // Clear the buffer
    recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0, 
                        (struct sockaddr *)&client_addr, &client_len);
    
    if (recv_len < 0) {
        perror("Error receiving filename");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Received request for file: %s\n", buffer);

    // Step 5: Try to open the requested file
    file = fopen(buffer, "r");
    if (file == NULL) {
        // If file is not found, send a "NOTFOUND" message to the client
        strcpy(buffer, "NOTFOUND");
        sendto(sockfd, buffer, strlen(buffer), 0, 
               (struct sockaddr *)&client_addr, client_len);
        printf("File not found. Sent NOTFOUND to client.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 6: Read and send the first word (HELLO) from the file
    memset(buffer, 0, MAX_BUFFER); // Clear the buffer
    if (fgets(buffer, MAX_BUFFER, file) == NULL) {
        printf("Error reading HELLO from file\n");
        fclose(file);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
    printf("Sending HELLO to client\n");

    send_len = sendto(sockfd, buffer, strlen(buffer), 0, 
                      (struct sockaddr *)&client_addr, client_len);
    if (send_len < 0) {
        perror("Error sending HELLO");
        fclose(file);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Step 7: Process WORD requests from the client
    while (1) {
        printf("Waiting for WORD request...\n");
        memset(buffer, 0, MAX_BUFFER); // Clear the buffer
        recv_len = recvfrom(sockfd, buffer, MAX_BUFFER, 0, 
                            (struct sockaddr *)&client_addr, &client_len);

        if (recv_len < 0) {
            perror("Error receiving WORD request");
            break;
        }

        printf("Received request: %s\n", buffer);

        // Check if the request starts with "WORD"
        if (strncmp(buffer, "WORD", 4) == 0) {
            memset(buffer, 0, MAX_BUFFER); // Clear the buffer
            if (fgets(buffer, MAX_BUFFER, file) != NULL) {
                buffer[strcspn(buffer, "\n")] = 0; // Remove newline character

                // Send the word to the client
                send_len = sendto(sockfd, buffer, strlen(buffer), 0, 
                                  (struct sockaddr *)&client_addr, client_len);
                if (send_len < 0) {
                    perror("Error sending word");
                    break;
                }

                // If the word is "FINISH", terminate the server
                if (strcmp(buffer, "FINISH") == 0) {
                    printf("Sent FINISH, closing server.\n");
                    fclose(file);
                    close(sockfd);
                    printf("Server terminated successfully\n");
                    exit(EXIT_SUCCESS);
                }
            } else {
                // If there's an error reading or EOF is reached
                printf("Error reading from file or reached EOF\n");
                fclose(file);
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        } else {
            printf("Received invalid request format: %s\n", buffer);
        }
    }

    // Step 8: Cleanup and close resources in case of errors
    fclose(file);
    close(sockfd);
    return EXIT_FAILURE;
}