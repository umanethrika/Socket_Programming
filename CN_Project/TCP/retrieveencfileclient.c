 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>

// Configuration constants
#define PORT 8080           // Server port number
#define CHUNK_SIZE 100      // Size of data chunks for file transfer
#define MAX_PATH 100        // Maximum length of filename

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[CHUNK_SIZE] = {0};
    char filename[MAX_PATH] = {0};
    char enc_filename[MAX_PATH] = {0};
    char key[27] = {0};     // 26 chars + null terminator
    char response[10] = {0};

    // Create TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    // Configure server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert localhost address to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed\n");
        return -1;
    }

    printf("Connected to server\n");

    // Main client loop
    while (1) {
        // Get user's intention to encrypt a file
        printf("\nDo you want to encrypt a file? (Yes/No): ");
        memset(response, 0, sizeof(response));
        scanf("%9s", response);
        
        // Convert response to lowercase for case-insensitive comparison
        for(int i = 0; response[i]; i++) {
            response[i] = tolower(response[i]);
        }

        // Handle termination request
        if (strcmp(response, "no") == 0) {
            send(sock, "No", 3, 0);
            break;
        }

        // Send continuation signal to server
        send(sock, "Yes", 3, 0);

        // Get and validate input filename
        while (1) {
            printf("Enter filename to encrypt: ");
            memset(filename, 0, MAX_PATH);
            scanf("%99s", filename);
            
            // Check if filename is too long
            if (strlen(filename) >= MAX_PATH - 5) { // Leave room for ".enc"
                printf("Filename too long\n");
                continue;
            }
            
            // Check if file exists
            FILE *file = fopen(filename, "r");
            if (!file) {
                printf("NOTFOUND %s\n", filename);
                continue;
            }
            fclose(file);
            break;
        }

        // Get and validate encryption key
        while (1) {
            printf("Enter encryption key (26 characters): ");
            memset(key, 0, sizeof(key));
            scanf("%26s", key);
            if (strlen(key) != 26) {
                printf("Error: Key must be exactly 26 characters\n");
                continue;
            }
            key[26] = '\0';
            break;
        }

        // Send encryption key to server
        send(sock, key, 26, 0);

        // Create output filename
        if (snprintf(enc_filename, MAX_PATH, "%s.enc", filename) >= MAX_PATH) {
            printf("Encrypted filename would be too long\n");
            continue;
        }

        // Read and send file content in chunks
        FILE *input_file = fopen(filename, "rb");
        size_t bytes_read;
        memset(buffer, 0, CHUNK_SIZE);
        
        while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, input_file)) > 0) {
            send(sock, buffer, bytes_read, 0);
            memset(buffer, 0, CHUNK_SIZE);
        }
        fclose(input_file);

        // Send end-of-file marker
        send(sock, "END\n", 4, 0);

        // Receive and save encrypted file
        FILE *output_file = fopen(enc_filename, "wb");
        if (!output_file) {
            perror("Failed to create output file");
            continue;
        }

        // Receive encrypted data in chunks
        size_t bytes_received;
        int end_marker_received = 0;
        memset(buffer, 0, CHUNK_SIZE);
        
        while ((bytes_received = recv(sock, buffer, CHUNK_SIZE, 0)) > 0) {
            if (bytes_received >= 4 && memcmp(buffer + bytes_received - 4, "END\n", 4) == 0) {
                fwrite(buffer, 1, bytes_received - 4, output_file);
                end_marker_received = 1;
                break;
            }
            fwrite(buffer, 1, bytes_received, output_file);
            memset(buffer, 0, CHUNK_SIZE);
        }
        
        fclose(output_file);

        if (!end_marker_received) {
            printf("Error: incomplete file transfer\n");
            remove(enc_filename);
            continue;
        }

        // Print success message
        printf("File encrypted successfully!\n");
        printf("Original file: %s\n", filename);
        printf("Encrypted file: %s\n", enc_filename);
    }

    // Clean up and close connection
    close(sock);
    printf("Connection closed\n");
    return 0;
}