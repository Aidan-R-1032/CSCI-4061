#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"
#include "swish_funcs.h"

#define CMD_LEN 512
#define PROMPT "@> "

int main(int argc, char **argv) {
    // Task 4: Set up shell to ignore SIGTTIN, SIGTTOU when put in background
    // You should adapt this code for use in run_command().
    struct sigaction sac;
    sac.sa_handler = SIG_IGN;
    if (sigfillset(&sac.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    strvec_t tokens;
    strvec_init(&tokens);
    job_list_t jobs;
    job_list_init(&jobs);
    char cmd[CMD_LEN];

    printf("%s", PROMPT);
    while (fgets(cmd, CMD_LEN, stdin) != NULL) {
        // Need to remove trailing '\n' from cmd. There are fancier ways.
        int i = 0;
        while (cmd[i] != '\n') {
            i++;
        }
        cmd[i] = '\0';
        //Utilizes the tokenize function in swish_funcs to split the commands into usable strings.
        //These are saved in the string vector "tokens"
        if (tokenize(cmd, &tokens) != 0) {
            printf("Failed to parse command\n");
            strvec_clear(&tokens);
            job_list_free(&jobs);
            return 1;
        }
        if (tokens.length == 0) {
            printf("%s", PROMPT);
            continue;
        }
        const char *first_token = strvec_get(&tokens, 0);
        if (first_token == NULL) {
            printf("Error Parsing first Token\n");
        }
        //handles the pwd command by uttilizing the already existing getcwd system call.
        //This will print the current working directory
        if (strcmp(first_token, "pwd") == 0) {
            char cwd[CMD_LEN];
            if(getcwd(cwd, CMD_LEN) == NULL){
                perror("getcwd");
            }
            else{
                printf("%s\n", cwd);
            }
        }
        else if (strcmp(first_token, "cd") == 0) {
            //Handles cd command by using the exisiting system calls getenv and chdir
            char *directory_name = strvec_get(&tokens, 1);
            if(directory_name == NULL){
                char env[CMD_LEN];
                if (getenv(env) == NULL){
                    perror("getenv");
                }
                if(chdir(env) == -1){
                    perror("chdir");
                }
            }
            else{
                if(chdir(directory_name) == -1){
                    perror("chdir");
                }
            }
        }

        else if (strcmp(first_token, "exit") == 0) {
            strvec_clear(&tokens);
            job_list_free(&jobs); //added in order to do proper cleanup, was not included in the original skeleton code
            break;
        }

        // Task 5: Print out current list of pending jobs
        else if (strcmp(first_token, "jobs") == 0) {
            int i = 0;
            job_t *current = jobs.head;
            while (current != NULL) {
                char *status_desc;
                if (current->status == JOB_BACKGROUND) {
                    status_desc = "background";
                } else {
                    status_desc = "stopped";
                }
                printf("%d: %s (%s)\n", i, current->name, status_desc);
                i++;
                current = current->next;
            }
        }

        // Task 5: Move stopped job into foreground
        else if (strcmp(first_token, "fg") == 0) {
            if (resume_job(&tokens, &jobs, 1) == -1) {
                printf("Failed to resume job in foreground\n");
            }
        }

        // Task 6: Move stopped job into background
        else if (strcmp(first_token, "bg") == 0) {
            if (resume_job(&tokens, &jobs, 0) == -1) {
                printf("Failed to resume job in background\n");
            }
        }

        // Task 6: Wait for a specific job identified by its index in job list
        else if (strcmp(first_token, "wait-for") == 0) {
            if (await_background_job(&tokens, &jobs) == -1) {
                printf("Failed to wait for background job\n");
            }
        }

        // Task 6: Wait for all background jobs
        else if (strcmp(first_token, "wait-all") == 0) {
            if (await_all_background_jobs(&jobs) == -1) {
                printf("Failed to wait for all background jobs\n");
            }
        }

        else {
            pid_t pid = fork();
            if(pid == -1) { //fork error case
                perror("fork");
            }
            int status;
            if(pid == 0){ //child, where commands are run
                if(run_command(&tokens) == -1){
                    return 1;
                }
            }
            else if (strcmp(strvec_get(&tokens, tokens.length - 1), "&") == 0){
                //parent case where a jopb needs to run in the background, indiciated by the presence of '&'
                //Removes the '&' and then sets the job status to JOB_BACKGROUND
                strvec_take(&tokens, tokens.length - 1);
            
                if (job_list_add(&jobs, pid, first_token, JOB_BACKGROUND) != 0) { 
                    printf("Error adding job into background\n");
                }   
            }
            else {
                //parent case where there is no background job indicated, aka job is in foreground
                //Sets the group, waits for the child, and then resets the group of the parent
                //If a job is stopped it is added to the job list with the JOB_STOPPED status
                if(tcsetpgrp(STDIN_FILENO, pid) == -1){
                    perror("tcsetpgrp");
                } 
                if (waitpid(-1, &status, WUNTRACED) == -1){
                    perror("waitpid");
                }
                if(tcsetpgrp(STDIN_FILENO, getpid()) == -1){
                    perror("tcsetpgrp");
                }
                if (WIFSTOPPED(status)) {
                    if(job_list_add(&jobs, pid, strvec_get(&tokens, 0), JOB_STOPPED) == -1) {
                        printf("Problem adding job to job list\n");
                    }
                }
            }
        }

        strvec_clear(&tokens);
        printf("%s", PROMPT);
    }

    return 0;
}
