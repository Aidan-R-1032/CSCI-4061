#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
int keep_going = 1;

// Handler for SIGINT - ensures proper cleanup
void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    // First command line arg is port to bind to as server
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    const char *server_port = argv[1];

    // Catch SIGINT so we can clean up properly
    struct sigaction sigact;
    sigact.sa_handler = handle_sigint;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = 0; // Note the lack of SA_RESTART
    if (sigaction(SIGINT, &sigact, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // Set up hints - we'll take either IPv4 or IPv6, TCP socket type
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // We'll be acting as a server
    struct addrinfo *server;

    // Set up address info for socket() and connect()
    int ret_val = getaddrinfo(NULL, server_port, &hints, &server);
    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        return 1;
    }
    // Initialize socket file descriptor
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1) {
        perror("socket");
        freeaddrinfo(server);
        return 1;
    }
    // Bind socket to receive at a specific port
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }
    freeaddrinfo(server);
    // Designate socket as a server socket
    if (listen(sock_fd, LISTEN_QUEUE_LEN) == -1) {
        perror("listen");
        close(sock_fd);
        return 1;
    }

    while (keep_going != 0) {
        // Wait to receive a connection request from client
        // Don't both saving client address information
        printf("Waiting for client to connect\n");
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno != EINTR) {
                perror("accept");
                close(sock_fd);
                return 1;
            } else {
                break;
            }
        }
        printf("New client connected\n");

        // Keep reading data from client until they close connection
        char buf[BUFSIZE];
        int bytes_read;
        while ((bytes_read = read(client_fd, buf, BUFSIZE)) > 0) {
            printf("Received original string: \"%s\"\n", buf);
            strncat(buf, "?", BUFSIZE - 1);
            printf("Replying with: \"%s\"\n", buf);
            if (write(client_fd, buf, strlen(buf) + 1) == -1) {
                // Close this client's connection, but keep accepting new clients
                perror("write");
                close(client_fd);
                break;
            }
        }
        printf("Client disconnected\n");
    }

    // Don't forget cleanup - reached even if we had SIGINT
    if (close(sock_fd) == -1) {
        perror("close");
        return 1;
    }
    return 0;
}
