#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER_SIZE 1024

void print_error_and_exit(const char *message) {
    perror(message);
    exit(1);
}

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX* create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_client_method();  // Use TLS protocol
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        print_error_and_exit("Unable to create SSL context");
    }
    return ctx;
}

int main() {
    char server_ip[16];
    int server_port;
    char message[BUFFER_SIZE];
    int sockfd;
    struct sockaddr_in server_addr;
    SSL_CTX *ctx;
    SSL *ssl;
    char buffer[BUFFER_SIZE];

    // Prompt user for server details
    printf("Enter the server IP address (e.g., 127.0.0.1): ");
    scanf("%s", server_ip);
    printf("Enter the server port number: ");
    scanf("%d", &server_port);
    getchar(); // To consume the newline character
    printf("Enter the message to send to the server: ");
    fgets(message, BUFFER_SIZE, stdin);
    message[strcspn(message, "\n")] = 0;  // Remove newline character

    // Initialize OpenSSL
    init_openssl();

    // Create SSL context
    ctx = create_context();

    // Create a TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        print_error_and_exit("Error creating socket");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        print_error_and_exit("Error connecting to server");
    }

    // Create SSL object and associate with the client socket
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);

    // Perform SSL handshake
    if (SSL_connect(ssl) <= 0) {
        print_error_and_exit("Error in SSL handshake");
    }

    // Send message to server
    if (SSL_write(ssl, message, strlen(message)) <= 0) {
        print_error_and_exit("Error sending data to server");
    }

    printf("Message sent to server: %s\n", message);

    // Receive the echoed message from server
    int bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytes_received <= 0) {
        print_error_and_exit("Error receiving data from server");
    }
    buffer[bytes_received] = '\0';  // Null-terminate the received message

    printf("Received echoed message from server: %s\n", buffer);

    // Clean up
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);
    cleanup_openssl();

    return 0;
}
