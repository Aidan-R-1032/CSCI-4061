#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    // Run equivalent of the command pipeline 'sort -n test_cases/resources/numbers.txt | tail -n 10'

    // TODO Create pipe
    int pipe_fds[2];
    if(pipe(pipe_fds) == -1){
        perror("pipe");
        return -1;
    }
    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        // TODO Insert any necessary cleanup
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return 1;
    } else if (child_pid == 0) {
        // TODO Close write end of pipe
        close(pipe_fds[1]);
        // TODO Run the 'tail' command, setting up input from pipe first
        dup2(pipe_fds[0], STDIN_FILENO);
        execlp("tail", "tail", "-n", "10", NULL);
        return 0; // Not reached on successful exec()
    } else {
        // TODO Parent closes read end of pipe
        close(pipe_fds[0]);
    }

    // TODO Run 'sort' command in original process, setting up output to pipe first
    dup2(pipe_fds[1], STDOUT_FILENO);
    execlp("sort", "sort", "-n", "test_cases/resources/numbers.txt", NULL);
    return 0; // Not reached on successful exec()
}
