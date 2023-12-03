#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEPT_LEN 5
#define NAME_LEN 128

struct server_request{
    char dept[DEPT_LEN];
    int course_num;
};

struct server_response{
    int lookup;
    char course_name[NAME_LEN];
};

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <Department> <Number>\n", argv[0]);
        return 1;
    }
    // You may uncomment the following lines if you would like to use them
    const char *department = argv[1];
    int number = atoi(argv[2]);

    // TODO Send a request for a course's title to UDP server
    // You will need to use getaddrinfo() first
    // Followed by the socket() system call
    // And finally sendto() to send a request and recvfrom() to get the reply

    // Pay close attention to the request/response formats explained in
    // QUESTIONS.txt -- you are encouraged to use structs to represent
    // requests and responses
    //
    // Don't forget about endianness!

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo *server;

    int ret_val = getaddrinfo("23.23.63.223", "4061", &hints, &server);
    if(ret_val != 0){
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        return 1;
    }
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if(sock_fd == -1){
        perror("socket");
        return -1;
    }

    struct server_request req;
    strcpy(req.dept, department);
    req.course_num = htonl(number);

    if(sendto(sock_fd, &req, sizeof(req), 0, server->ai_addr, server->ai_addrlen) == -1){
        perror("sendto");
        return -1;
    }

    struct server_response res;
    if(recvfrom(sock_fd, &res, sizeof(res), 0, NULL, NULL) == -1){
        perror("recvfrom");
        return -1;
    }
    res.lookup = ntohl(res.lookup);
    freeaddrinfo(server);
    if(res.lookup == 0){
        printf("Course Title: %s\n", res.course_name);
    }
    else{
        printf("Course Not Found\n");
    }
    close(sock_fd);
    return 0;
}
