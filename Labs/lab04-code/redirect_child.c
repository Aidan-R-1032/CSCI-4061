// redirect_child.c: starts a child process which will print into a
// file instead of onto the screen. Uses dup2(), fork(), and wait()

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) { // check for at least 1 command line arg
        printf("Usage: %s <childfile>\n", argv[0]);
        return 1;
    }

    // Uncomment lines below to use specified output file and command-line args in child process
    char *output_file = argv[1]; // output file that child process will print into
    char *child_argv[] = {"wc", "test_cases/resources/nums.txt", NULL}; // child command/arguments to execute;

    int out_fd = open(output_file, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    // TODO: Spawn a child process, which will exec the "wc" command with the arguments in 'child_argv'
    // Redirect the output of the command to 'output_file'
    int pid = fork();
    if(pid < 0){
        perror("fork");
        return -1;
    }
    else if (pid == 0){
        dup2(out_fd, STDOUT_FILENO);
        execvp("wc", child_argv);
    }
    else{
        int status_code = -1;
        wait(&status_code);
        if(WIFEXITED(status_code)){
            printf("Child complete, return code %d\n", status_code);
        }
        else{
            printf("Child exited abnormally\n");
        }
    }

    // TODO: In the parent, wait for the child and ensure it terminated normally using wait macros
    // Print "Child complete, return code <status_code>" if child terminated normally, replacing
    //    <status_code> with the child's numerical status code
    // Print "Child exited abnormally" if child terminated abnormally

    return 0;
}
