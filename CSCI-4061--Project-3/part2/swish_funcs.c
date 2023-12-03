#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include "string_vector.h"
#include "swish_funcs.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline. You should make
 * make use of the provided 'run_command' function here.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipes: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipes, int in_idx, int out_idx) {

    //check for indexing errors
    if(in_idx < -1 || in_idx > n_pipes || out_idx < -1 || out_idx > n_pipes || in_idx == out_idx){
        printf("indexing error\n");
        return -1;
    }

    //close the pipes not in use
    for(int i = 0; i < n_pipes; i++){
        if(i != in_idx && i != out_idx){ 
            if(close(pipes[i]) == -1){
                perror("close");
                return -1;
            }
        }
    }

    //backups
    int in_back;
    if((in_back = dup(STDIN_FILENO)) == -1){
        perror("dup");
        return -1;
    }
    int out_back;
    if((out_back = dup(STDOUT_FILENO)) == -1){
        perror("dup");
    }

    if(in_idx == -1 ) { //case where the it is the last command, output to stdout
        if(dup2(pipes[out_idx], STDOUT_FILENO) == -1){
        perror("dup2");
        return -1;
    }
    }
    else { //standard case
            if(dup2(pipes[in_idx], STDIN_FILENO) == -1){
            perror("dup2");
            return -1;
        }
    }
    

    if(out_idx == -1) { //case where it is the first command, input from stdin
        if(dup2(pipes[in_idx], STDIN_FILENO) == -1){
            perror("dup2");
            return -1;
        }
    }
    else { //standard case
        if(dup2(pipes[out_idx], STDOUT_FILENO) == -1){
            perror("dup2");
            return -1;
        }
    }
    
    if(run_command(tokens) == -1) { //run the command
        if(dup2(in_back, STDIN_FILENO) == -1){ //reset backups in the case of failure
            perror("dup2");
            return -1;
        }
        if(dup2(out_back, STDOUT_FILENO) == -1){
            perror("dup2");
            return -1;
        }
    }
    return 0;
}

/*
 * Function that takes in the a string vector and will running of the pipeline.
 * Slices each of the individual commands and their arguments based on the presence of "|"
 * It then runs the commands using the run_piped_commands() helper function
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * 
 * Returns 0 on success or -1 on error.
 */
int run_pipelined_commands(strvec_t *tokens) {
    //finds the number of times the pipe character appears
    //will be used for the number of forks later
    int num_occurrences;
    if((num_occurrences = strvec_num_occurrences(tokens, "|")) == -1){
        printf("strvec_num_occurences\n");
        return -1;
    }

    
    int pipes[2 * num_occurrences]; //create array of pipes
    int occurences[num_occurrences]; //create occurence array
    int occur_idx = 0;

    //fill the "pipes" array with actual pipes using pipe syscall
    for(int i = 0; i < num_occurrences; i++) {
        if(pipe(pipes + i*2) == -1){ 
            perror("pipe");
            return -1;
        }
    }

    //find the occcurences of | and put the indexes in an array
    //for usage during slicing later
    for(int i = 0; i < tokens->length; i++){
        if(strcmp(strvec_get(tokens, i), "|") == 0) {
            occurences[occur_idx] = i;
            occur_idx++;
        }
    }

    //fork for the number of pipes + 1.
    //i is the command number that is being run
    for(int i = 0; i <= num_occurrences; i++) { 
        pid_t pid = fork();
        if(pid == -1){
            perror("fork");
            return -1;
        }
        if(pid == 0){ //child
            strvec_t command;
            //take a slice of tokens, this will be the command that needs to be ran.
            //based on what command number is being used
            if(i == num_occurrences){ //last command
                if(strvec_slice(tokens, &command, occurences[i - 1] + 1, tokens->length + 1) == -1) {
                    printf("slice\n");
                    return -1;
                }
            } 
            else if(i == 0) { //first command
                if(strvec_slice(tokens, &command, 0, occurences[0]) == -1) {
                    printf("slice\n");
                    return -1;
                }
            } 
            else { //all commands that are not first or last
                if(strvec_slice(tokens, &command, occurences[i - 1] + 1, occurences[i]) == -1) {
                    printf("slice\n");
                    return -1;
                }
            }

            if(i == 0) { // i is the cmd number, starts at cmd0
                if(run_piped_command(&command, pipes, 2 * num_occurrences, -1,  1) == -1){
                    return -1;
                }
            }
            else if (i == num_occurrences) { //last command case
                if(run_piped_command(&command, pipes, 2 * num_occurrences, 2*(i-1),  -1) == -1){
                    return -1;
                }
            }
            else { //in between cases
                if(run_piped_command(&command, pipes, 2 * num_occurrences, 2*(i - 1),  2*(i - 1) + 3) == -1){
                    return -1;
                }
            }
            //cleanup
            strvec_clear(&command);
            exit(0);
        }
    }

    //close all of the pipes, 2*i for read end, 2*i + 1 for write end
    for(int i = 0; i < num_occurrences; i++) {
        if(close(pipes[2*i]) == -1 || close(pipes[2*i + 1]) == -1){
            perror("close");
            return -1;
        }
    }
    //wait for n+1 processes, this is the number of commands run
    for(int i = 0; i < num_occurrences + 1; i++){
        int status;
        if(wait(&status) == -1) {
            perror("wait");
            return -1;
        }
    }
    return 0;
}
