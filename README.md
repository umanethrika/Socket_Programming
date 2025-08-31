# Socket_Programming
UDP and TCP low level programming with Wireshark Analysis

Networked File Transfer and Encryption

This repository contains two projects implemented in C using POSIX sockets:

UDP File Transfer (Assignment 2) – File transfer using datagram sockets.

TCP File Encryption (Assignment 3) – File encryption service using substitution cipher and TCP sockets.

Both projects include Wireshark analysis for understanding protocol behavior.

Compile using:
gcc -o wordserver wordserver.c
gcc -o wordclient wordclient.c
gcc -o doencfileserver doencfileserver.c
gcc -o retrieveencfileclient retrieveencfileclient.c

1. Create Input Files


22CS10045_File1.txt

HELLO
CS31206
CS39006
CS31208
FINISH


The first word must be HELLO

The last word must be FINISH

Any number of words can exist in between

2. Run the Server
./wordserver

Server will listen on port 8080.

3. Run the Client
./wordclient 22CS10045_File1.txt output.txt


First argument → input filename to request from server

Second argument → output filename to store received contents

4. Expected Behavior

If file exists → Client receives all words (HELLO → FINISH) and stores them.

If file does not exist → Client prints FILE NOT FOUND.

Assignment 3: TCP File Encryption
1. Create Input Files

Inside assignment3_tcp/sample_plaintext_files/, create plaintext files (only alphabets, spaces, newlines).

dummy.txt

HELLO
ABC
CSE
IIT Kharagpur

2. Run the Server
./doencfileserver


Server will listen on port 8080.

3. Run the Client
./retrieveencfileclient


Client will ask interactively:

Do you want to encrypt a file? (Yes/No)

Enter filename to encrypt: (must exist in directory)

Enter encryption key (26 characters): (e.g., DEFPRTVWLMZAYGHQSIUJXKBCNO)

4. Expected Behavior

Server stores the received file temporarily as <clientIP>.<clientPort>.txt.

Encrypts it using the substitution cipher.

Sends back encrypted file, saved as <filename>.enc by the client.

Example:

Input: dummy.txt

Output: dummy.txt.enc

5. Exit

Enter No when prompted, and the client will close the connection gracefully.
