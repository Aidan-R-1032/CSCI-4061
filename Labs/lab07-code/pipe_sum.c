#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int write_sums_to_pipe(const char *file_name, int pipe_write_fd) {
    FILE *f = fopen(file_name, "r");
    if (f == NULL) {
        perror("fopen");
        return 1;
    }

    int sum = 0;
    int n;
    while (fscanf(f, "%d", &n) > 0) {
        sum += n;
        // TODO write cumulative sum to pipe
        write(pipe_write_fd, &sum, sizeof(int));
    }

    int ret_val = 0;
    if (ferror(f)) {
        perror("Failed to read file");
        ret_val = 1;
    }
    fclose(f);
    return ret_val;
}

int read_sums_from_pipe(int pipe_read_fd) {
    // TODO read all sums from pipe
    int bytes_read = -1;
    int num = -1;
    while((bytes_read = read(pipe_read_fd, &num, sizeof(int)) == sizeof(int))){
        printf("Cumulative Sum: %d\n", num);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <numbers_file>\n", argv[0]);
        return 1;
    }
    // Uncomment line below if you would like to use it
    const char *file_name = argv[1];

    // TODO set up pipe file descriptors
    int pipe_fds[2];
    if(pipe(pipe_fds) == -1){
        perror("pipe");
        return -1;
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        return 1;
    }

    else if (child_pid == 0) {
        // TODO write code to do the following:
        // 1. Close unused pipe file descriptors
        // 2. Call 'write_sums_to_pipe' with appropriate arguments
        // 3. Close remaining pipe file descriptors
        close(pipe_fds[0]);
        write_sums_to_pipe(file_name, pipe_fds[1]);
        close(pipe_fds[1]);
    }

    else {
        // TODO write code to do the following:
        // 1. Close unused pipe file descriptors
        // 2. Call 'read_sums_from_pipe' with appropriate arguments
        // 3. Close remaining pipe file descriptors
        close(pipe_fds[1]);
        read_sums_from_pipe(pipe_fds[0]);
        close(pipe_fds[0]);
    }

    return 0;
}
