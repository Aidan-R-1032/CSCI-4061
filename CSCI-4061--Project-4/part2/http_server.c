#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

typedef struct{
    connection_queue_t queue;
    int fd;
} producer_args;

void *consumer_thread_func(void *arg) {
//create the queue
    connection_queue_t *queue = (connection_queue_t *) arg;
    int client_fd;
    //loop while shutdown has not been enacted
    while(queue->shutdown == 0){
        if(queue->shutdown == 0){
            client_fd = connection_dequeue(queue);//dequeue a fd, this will wait for a fd to be availible
        }
        //if fd doesnt exist, return, for the case of bad file descriptors
        if(client_fd == -1){
            close(client_fd);
            return NULL;
        }
        //for the resource name
        char resource_name[BUFSIZE];
        memset(resource_name, 0, BUFSIZE);
        //read the full http request
        if(read_http_request(client_fd, resource_name) == -1){
            close(client_fd);
            continue;
        }
        //setup the directory for the response
        char directory[BUFSIZE];
        strcpy(directory, queue->directory);
        strcat(directory, resource_name);
	//write the response
        if(write_http_response(client_fd, directory) == -1){
            close(client_fd);
            continue;
        }
    }
    close(client_fd);
    return NULL;
}

int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }

    struct sigaction sigact;
    sigact.sa_handler = handle_sigint;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = 0; // Note the lack of SA_RESTART
    if (sigaction(SIGINT, &sigact, NULL) == -1) {
        perror("sigaction");
        return 1;
    }
   
    connection_queue_t queue;
    connection_queue_init(&queue);
    queue.directory = argv[1];

    // Uncomment the lines below to use these definitions:
    const char *port = argv[2];

    //socket and connection stuff
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; //ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM; //TCP connection
    hints.ai_flags = AI_PASSIVE; //for binding to a port, caused a lot of problems when not present
    
    struct addrinfo *server;
    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        return 1;
    }
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol); //the socket
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen) == -1) { //bind
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }
    freeaddrinfo(server);
    if (listen(sock_fd, LISTEN_QUEUE_LEN) == -1) {//and listen
        perror("listen");
        close(sock_fd);
        return 1;
    }


    //signal stuff
    sigset_t fill_set;
    sigfillset(&fill_set);
    sigset_t old_set;
    sigprocmask(SIG_BLOCK, &fill_set, &old_set);//save the old mask
    pthread_t c_threads[N_THREADS];
    //spawn N_Threads number of threads
    for(int i = 0; i < N_THREADS; i++){
        pthread_create(&c_threads[i], NULL, consumer_thread_func, &queue);
    }
    //restore the old signal mask so sigint will only be delivered to the main process
    sigprocmask(SIG_SETMASK, &old_set, NULL); 

    //break on a delivery of sigint
    while(keep_going != 0){
    //not sure if this print statement is needed
        //printf("Waiting for client to connect\n");
        int client_fd = accept(sock_fd, NULL, NULL); //accept
        if (client_fd == -1) { //check the file descriptor
            if (errno != EINTR) {
                perror("accept");
                close(sock_fd);
                return 1;
            } else {
                break;
            }
        }
        //add the connection to the queue for processing
        connection_enqueue(&queue, client_fd);
    }
     //once the loop is broken shutdown sequence must occur
    connection_queue_shutdown(&queue); // call shutdown
    //join the threads
    for(int i = 0; i < N_THREADS; i++){
        pthread_join(c_threads[i], NULL);
    }
    //close the socket
    close(sock_fd);
    //free the queue
    connection_queue_free(&queue);
    return 0;
}
