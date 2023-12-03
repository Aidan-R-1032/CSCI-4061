#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_CHILDREN 7
#define RAND_SEED 4061

int main() {
    int child_values[] = {4, 8, 15, 16, 23, 42, 108};
    srand(RAND_SEED);

    // Create a pipe for each child process
    int pipe_fds[2 * NUM_CHILDREN];
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pipe_fds + 2*i) == -1) {
            perror("pipe");
            for (int j = 0; j < 2*i; j++) {
                close(pipe_fds[j]);
            }
            return 1;
        }
    }

    // Launch child processes, each writing to pipe after random delay
    for (int i = 0; i < NUM_CHILDREN; i++) {
        int delay = rand() % 13;

        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork");
            for (int j = 0; j < 2 * NUM_CHILDREN; j++) {
                close(pipe_fds[j]);
            }
            return 1;
        }

        else if (child_pid == 0) {
            sleep(delay);

            int ret_val = 0;
            if (write(pipe_fds[2*i + 1], child_values + i, sizeof(int)) == -1) {
                perror("write");
                ret_val = 1;
            }
            for (int j = 0; j < 2 * NUM_CHILDREN; j++) {
                if (close(pipe_fds[j]) == -1) {
                    ret_val = 1;
                }
            }
            return ret_val;
        }
    }

    // TODO Change this code to read from pipes as soon as data is available
    // Use the poll() system call
    struct pollfd *pollfds = malloc(sizeof(struct pollfd) * NUM_CHILDREN);
    for(int i = 0; i < NUM_CHILDREN; i++){
        pollfds[i].fd = pipe_fds[2*i];
        pollfds[i].events = POLLIN | POLLHUP;
    }
    int num_remaining = NUM_CHILDREN;
    while(num_remaining){
        if(poll(pollfds, NUM_CHILDREN, -1) > 0){
            for (int i = 0; i < NUM_CHILDREN; i++) {
                int result;
                if(pollfds[i].revents & POLLIN){
                    if (read(pipe_fds[2*i], &result, sizeof(int)) == -1) {
                        perror("read");
                        for (int j = 0; j < 2 * NUM_CHILDREN; j++) {
                            close(pipe_fds[j]);
                        }
                        free(pollfds);
                        return 1;
                    }
                    else if(pipe_fds[2*i] != -1){
                        pipe_fds[2*i] = -1;
                        num_remaining--;
                        printf("Child %d Result: %d\n", i, result);
                    }
                }
            }
        }
    }
    free(pollfds);
    // End TODO block

    int ret_val = 0;
    for (int i = 0; i < 2 * NUM_CHILDREN; i++) {
        if (close(pipe_fds[i]) == -1) {
            ret_val = 1;
        }
    }
    // Wait for all children -- not strictly necessary but good practice
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (wait(NULL) == -1) {
            perror("wait");
            ret_val = 1;
        }
    }
    return ret_val;
}
