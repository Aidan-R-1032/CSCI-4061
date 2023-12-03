#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_WORD_LEN 25

/*
 * Counts the number of occurrences of words of different lengths in a text
 * file and stores the results in an array.
 * file_name: The name of the text file from which to read words
 * counts: An array of integers storing the number of words of each possible
 *     length.  counts[0] is the number of 1-character words, counts [1] is the
 *     number of 2-character words, and so on.
 * Returns 0 on success or -1 on error.
 */
int count_word_lengths(const char *file_name, int *counts) {
    FILE* fd = fopen(file_name, "r");
    if(fd == NULL){                 //fopen error branch
        perror("fopen");
        return -1;
    }
        int length = 0;             // length of the current word
        char file_char;             // current character in the file
        int EOF_not_reached = 1;    // 1 means the EOF has not been reached, 0 otherwise
        while(EOF_not_reached){
            file_char = (char) fgetc(fd);
            if(ferror(fd)){         // error check for fgetc - it returns EOF on error AND when at EOF, so need to check
                perror("fgetc");
                return -1;
            }
            while(file_char != EOF && file_char != ' ' && file_char != '\n'){ //keep increasing lenght until EOF, space, or newline
                length++;
                file_char = (char) fgetc(fd);
                if(ferror(fd)){         // error check for fgetc - it returns EOF on error AND when at EOF, so need to check
                    perror("fgetc");
                    return -1;
                }
            }
            if(length > 0){
                counts[length - 1] += 1;    // increment appropriate array value
            }
            length = 0;     // reset length
            if (feof(fd)){  // check to see if at EOF
                EOF_not_reached = 0;
            }
        }
    if(fclose(fd) != 0){
        perror("fclose");
        return -1;
    }
    return 0;
}

/*
 * Processes a particular file (counting the number of words of each length)
 * and writes the results to a file descriptor.
 * This function should be called in child processes.
 * file_name: The name of the file to analyze.
 * out_fd: The file descriptor to which results are written
 * Returns 0 on success or -1 on error
 */
int process_file(const char *file_name, int out_fd) {
    int counts[MAX_WORD_LEN];   // array containing word counts
    for(int i = 0; i < MAX_WORD_LEN; i++){  //initializes array to all zeros
        counts[i] = 0;
    }
    if(count_word_lengths(file_name, counts) == -1){    //error checks count_word_lengths
        return -1;
    }
    if (write(out_fd, counts, sizeof(int) * MAX_WORD_LEN) == -1){   //error checks write
        perror("write");
    return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {    // No files to consume, return immediately
        return 0;
    }
    int pipe_fds[2];
    int results[MAX_WORD_LEN];
    for(int i = 0; i < MAX_WORD_LEN; i++){  //initialize results array
        results[i] = 0;
    }
    int temp_result = 0;
    if(pipe(pipe_fds) == -1){   // pipe error checking
        perror("pipe");
        return -1;
    }
    pid_t pid;
    for(int i = 1; i < argc; i++){
        if((pid = fork()) == -1){   // create a process for each argument & error checks
            perror("fork");
            return -1;
        }
        if (pid == 0){
            if(close(pipe_fds[0]) == -1){   // indicate child never reads
                perror("close");
                return -1;
            }     
            if(process_file(argv[i], pipe_fds[1]) == -1){
                return -1;
            }
            if(close(pipe_fds[1]) == -1){   // indicate to the reader that no more info is being written
                perror("close");
                return -1;
            }
            exit(0);
        }
    }  
    if (pid != 0){
        if(close(pipe_fds[1]) == -1){   // indicates a parent never writes
            if(close(pipe_fds[0]) == -1){
                perror("close");
                return -1;
            }
        }     
        for(int i = 1; i < argc; i++){
            if(wait(NULL) == -1){   //wait for a process to finish
                perror("wait");
                return -1;
            }
        }
        for(int i = 1; i < argc; i++) {
            for(int i = 0; i < MAX_WORD_LEN; i++){  //store a processes word count in results array
                if(read(pipe_fds[0], &temp_result, sizeof(int)) == -1){
                    perror("read");
                    return -1;
                }
                results[i] += temp_result;  // add to results array
            }
        }
        if(close(pipe_fds[0]) == -1){
            perror("close");
            return -1;
        }
        for (int i = 1; i <= MAX_WORD_LEN; i++) {   // display results
            printf("%d-Character Words: %d\n", i, results[i - 1]);
        }
    }
    return 0;
}
