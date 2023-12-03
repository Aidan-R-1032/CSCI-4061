#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }

    // setup signal handling proceedure(s)
    struct sigaction sigact;
    sigact.sa_handler = handle_sigint;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    if (sigaction(SIGINT, &sigact, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    const char *port = argv[2];

    // get necessary connection information
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server;
    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        return 1;
    }

    // create and bind to a socket
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }

    // free the connection information - not needed anymore
    freeaddrinfo(server);

    // designates sock_fd as a passive (server) socket
    if (listen(sock_fd, LISTEN_QUEUE_LEN) == -1) {
        perror("listen");
        close(sock_fd);
        return 1;
    }

    // keep_going loop - loop exits upon receiving a SIGINT
    while(keep_going != 0){

        // Waits for a new client to request a TCP connection with the calling process
        int client_fd = accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            if (errno != EINTR) {   // error checking
                perror("accept");
                close(sock_fd);
                return 1;
            } else {
                break;
            }
        }
        char resource_name[BUFSIZE];
        memset(resource_name, 0, BUFSIZE);

        // call to read_http_request: -1 is received upon error
        if(read_http_request(client_fd, resource_name) == -1){
            continue;
        }

        // create a string to locate the path to the resource
        char directory[BUFSIZE];
        memset(directory, 0, BUFSIZE);
        strcpy(directory, argv[1]);
        strcat(directory, resource_name);
        
        // call to write_http_response: -1 is received upon error
        if(write_http_response(client_fd, directory) == -1){
            continue;
        }
    }
    // closing a socket represents terminating its connections 
    // (if there are any) and then close the file descriptor that
    // enables the message exchanging
    close(sock_fd);
    return 0;
}
