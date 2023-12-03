#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char** argv){
    int fd = open("out.txt", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR);
    for(int i = 1; i < argc; i++){//one process for each command
        pid_t id = fork();
        if(id == 0){
            dup2(fd, STDOUT_FILENO);//redirects output to out.txt
            execlp(argv[i], argv[i], NULL);//execute commands in this path execl(p) and individually exec(l)p
        }
    }
    close(fd);
    for(int i = 1; i < argc; i++){
        wait(NULL);//waits for each process to finish
    }
    printf("Commands completed\n");//outputs to termial
    return 0;
}