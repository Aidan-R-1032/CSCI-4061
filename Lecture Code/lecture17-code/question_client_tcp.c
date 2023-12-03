#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 512

int main(int argc, char **argv) {
    // Server host and port as command-line args
    if (argc != 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        return 1;
    }
    const char *server_host = argv[1];
    const char *server_port = argv[2];

    // Set up hints - we'll take either IPv4 or IPv6, TCP socket type
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *server;

    // Set up address info for socket()
    int ret_val = getaddrinfo(server_host, server_port, &hints, &server);
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

    // Connect to Server
    if (connect(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("connect");
        freeaddrinfo(server);
        return 1;
    }
    freeaddrinfo(server);

    char buf[BUFSIZE];
    while (fgets(buf, BUFSIZE, stdin) != NULL) {
        // Remove trailing '\n'
        int i = 0;
        while (buf[i] != '\n') {
            i++;
        }
        buf[i] = '\0';

        // Write question string to server
        if (write(sock_fd, buf, strlen(buf) + 1) == -1) {
            perror("write");
            close(sock_fd);
            return 1;
        }

        // Read reply back from server
        if (read(sock_fd, buf, BUFSIZE) == -1) {
            perror("read");
            close(sock_fd);
            return 1;
        }
        printf("Server Reply: \"%s\"\n", buf);
    }

    if (close(sock_fd) == -1) {
        perror("close");
        return 1;
    }
    return 0;
}
