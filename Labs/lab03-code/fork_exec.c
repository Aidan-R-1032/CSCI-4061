#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int child_status = -1;

    // TODO Fork a child process to run the command "cat sample.txt"
    // The parent should wait and print out the child's exit code
    int pid = fork();
    if(pid == -1){
        perror("fork");
        return -1;
    }
    if(pid == 0){
        execlp("cat", "cat", "sample.txt", NULL);
    }
    else{
        wait(&child_status);
    }

    printf("Child exited with status %d\n", child_status);
    return 0;
}
