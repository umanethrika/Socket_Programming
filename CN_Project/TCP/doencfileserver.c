

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Configuration constants
#define PORT 8080           // Server port number
#define CHUNK_SIZE 100      // Size of data chunks for file transfer
#define MAX_PATH 100        // Maximum length of filename

/**
 * Encrypts a single character using the provided substitution key
 * @param c The character to encrypt
 * @param key The 26-character substitution key
 * @return The encrypted character
 */
char encrypt_char(char c, const char* key) {
    if ((c >= 'A' && c <= 'Z')) {
        return key[c - 'A'];                    // Encrypt uppercase letters
    } else if (c >= 'a' && c <= 'z') {
        return key[c - 'a'] + ('a' - 'A');      // Encrypt lowercase letters
    }
    return c;                                   // Return unchanged if not a letter
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[CHUNK_SIZE] = {0};
    char key[27] = {0};                        // 26 chars + null terminator
    int should_terminate = 0;

    // Create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure socket to reuse address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;      // Accept connections on all network interfaces
    address.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {            // Queue up to 3 connections
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Main server loop
    while (!should_terminate) {
        // Accept new client connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Get and log client connection details
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(address.sin_port);
        printf("New client connected: %s:%d\n", client_ip, client_port);

        // Handle client requests
        while (1) {
            memset(buffer, 0, CHUNK_SIZE);
            // Receive continuation flag from client
            char continue_flag[4] = {0};
            ssize_t flag_recv = recv(new_socket, continue_flag, 3, MSG_WAITALL);
            continue_flag[3] = '\0';

            // Check if client wants to continue or disconnect
            if (flag_recv <= 0 || strcmp(continue_flag, "No") == 0) {
                printf("Client disconnecting...\n");
                break;
            }

            // Receive encryption key from client
            memset(key, 0, sizeof(key));
            ssize_t key_received = recv(new_socket, key, 26, MSG_WAITALL);
            if (key_received != 26) {
                printf("Server terminated\n");
                break;
            }
            key[26] = '\0';

            // Create temporary filenames
            char temp_filename[MAX_PATH] = {0};
            char enc_filename[MAX_PATH] = {0};
            
            if (snprintf(temp_filename, MAX_PATH, "%s.%d.txt", client_ip, client_port) >= MAX_PATH ||
                snprintf(enc_filename, MAX_PATH, "%s.enc", temp_filename) >= MAX_PATH) {
                printf("Filename too long\n");
                continue;
            }

            // Open temporary file for writing received data
            FILE *temp_file = fopen(temp_filename, "wb");
            if (!temp_file) {
                perror("Failed to create temporary file");
                continue;
            }

            // Receive and write file content
            size_t bytes_received;
            int end_marker_received = 0;
            
            while ((bytes_received = recv(new_socket, buffer, CHUNK_SIZE, 0)) > 0) {
                if (bytes_received >= 4 && memcmp(buffer + bytes_received - 4, "END\n", 4) == 0) {
                    fwrite(buffer, 1, bytes_received - 4, temp_file);
                    end_marker_received = 1;
                    break;
                }
                fwrite(buffer, 1, bytes_received, temp_file);
                memset(buffer, 0, CHUNK_SIZE);
            }
            
            fclose(temp_file);

            if (!end_marker_received) {
                printf("Error: incomplete file transfer\n");
                remove(temp_filename);
                continue;
            }

            // Perform file encryption
            FILE *input = fopen(temp_filename, "r");
            FILE *output = fopen(enc_filename, "w");
            if (!input || !output) {
                perror("File operation failed");
                if (input) fclose(input);
                if (output) fclose(output);
                remove(temp_filename);
                continue;
            }

            // Encrypt file character by character
            int c;
            while ((c = fgetc(input)) != EOF) {
                fputc(encrypt_char(c, key), output);
            }
            fclose(input);
            fclose(output);

            // Send encrypted file back to client
            FILE *enc_file = fopen(enc_filename, "rb");
            if (!enc_file) {
                perror("Failed to open encrypted file");
                remove(temp_filename);
                remove(enc_filename);
                continue;
            }

            // Send file in chunks
            memset(buffer, 0, CHUNK_SIZE);
            while ((bytes_received = fread(buffer, 1, CHUNK_SIZE, enc_file)) > 0) {
                send(new_socket, buffer, bytes_received, 0);
                memset(buffer, 0, CHUNK_SIZE);
            }
            fclose(enc_file);

            // Send end-of-file marker
            send(new_socket, "END\n", 4, 0);
            printf("File successfully encrypted and sent back to client.\n");

            // Clean up temporary files
            remove(temp_filename);
            remove(enc_filename);
        }

        // Clean up client connection
        close(new_socket);
        printf("Client disconnected: %s:%d\n", client_ip, client_port);
        close(server_fd);
        exit(0);
    }

    //close(server_fd);
    return 0;
}
