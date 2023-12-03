#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"
#include "swish_funcs.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens)
{

    char *token = strtok(s, " ");
    if (token == NULL){ //error checking strtok
        return -1;
    }
    while(token != NULL){   //keep looping until strtok returns NULL
        if (strvec_add(tokens, token) < 0){ //error checking strvec_add & adding tokens
            return -1;
        }
        token = strtok(NULL, " ");
    }
    return 0;
}

int arrow_redirect(char* arrow, const int file_desc) { //helper function used to redirect I/O - returns -1 on error & 0 on success
    if(strcmp(arrow, "<") == 0) { // redirects inut from given file
        if (dup2(file_desc, STDIN_FILENO) == -1) {  // error checks dup2 while also redirecting input
            perror("dup2");
            close(file_desc);
            return -1;
        }
        return 0;
    } // > - redirects to given file with truncation of the file, >> - appends instead of truncation
    else if(strcmp(arrow, ">") == 0 || strcmp(arrow, ">>") == 0 ) { 
        if (dup2(file_desc, STDOUT_FILENO) == -1) { // error checks dup2 while also redirecting output
            perror("dup2");
            close(file_desc);
            return -1;
        }
        return 0;
    }
    return -1;
}

int run_command(strvec_t *tokens)
{
    char *args[MAX_ARGS];                   // array holding arguments passed in from tokens
    int size = tokens->length;              // number of arguments passed in
    int i;                                  // used for looping in either case
   
   if(setpgid(0, getpid()) == -1) {         // error checks setpgid & sets the process group
        perror("setpgid");
        return -1;
    }
   
    struct sigaction sac2;                  // used for signals
    sac2.sa_handler = SIG_DFL;
    if (sigfillset(&sac2.sa_mask) == -1) {  // error cheks sigfillset & attempts to set the sigfillset field of sac2
        perror("sigfillset");
        return -1;
    }
    sac2.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac2, NULL) == -1 || sigaction(SIGTTOU, &sac2, NULL) == -1) {   //error checks & takes action on receipt of relevant signals
        perror("sigaction");
        return -1;
    }

    

    int cur_redir_idx;                      // used inside redirect loop
    int in_file_descriptor = -1; 
    int backup_input = dup(STDIN_FILENO);   // used for creating backups of STDIN
    int out_file_descriptor = -1;
    int backup_output = dup(STDOUT_FILENO); // used for creating backups of STDOUT
    int redirection_index;                  // index of '<', '>' , '>>' operator
    int redirect_indices[4];                // holds indexes of redirection arguments
    int redirect_op_total = 0;              // number of redirection arguments passed
    int redirected_input = 0;               // 0 if input remains unchanged, 1 otherwise
    int redirected_output = 0;              // 0 if output remains unchanged, 1 otherwise
    if((redirection_index = strvec_find(tokens, "<")) != -1){   //branch entered upon "<" passed as an argument
        in_file_descriptor = open(strvec_get(tokens, redirection_index + 1), O_RDONLY); //open input from file
        if(in_file_descriptor == -1){   //error checking open
            perror("Failed to open input file");
            return -1;
        }
        redirect_indices[redirect_op_total] = redirection_index;    // add the index of "<" argument to redirection_indices array
        redirect_op_total++;                                        // increment the total number of redirection arguments
        redirect_indices[redirect_op_total] = redirection_index + 1;// add the index of the redirected file 
        redirect_op_total++;                                        // increments the total number of redirection arguments
        if (arrow_redirect("<", in_file_descriptor) == -1){         // redirects input & error checks
            printf("could not redirect input\n");
            return -1;
        }
        redirected_input = 1;                                       // indicate that input was redirected
    }
    if((redirection_index = strvec_find(tokens, ">")) != -1){   //branch entered upon ">" passed as an argument
        out_file_descriptor = open(strvec_get(tokens, redirection_index + 1), O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);//open output from file
        if(out_file_descriptor == -1){  //error checking open
            perror("Failed to open output file");
            return -1;
        }
        redirect_indices[redirect_op_total] = redirection_index;    // add the index of ">" argument to redirection_indices array
        redirect_op_total++;                                        // increment the total number of redirection arguments
        redirect_indices[redirect_op_total] = redirection_index + 1;// add the index of the redirected file 
        redirect_op_total++;                                        // increments the total number of redirection arguments
        if(arrow_redirect(">", out_file_descriptor) == -1){         // redirects output & error checks
            printf("could not redirect output\n");
            return -1;
        }
        redirected_output = 1;                                      //indicate that output was redirected
    }
    else if((redirection_index = strvec_find(tokens, ">>")) != -1){ //branch entered upon ">>" passed as an argument
        out_file_descriptor = open(strvec_get(tokens, redirection_index + 1), O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);//open output from file
        if(out_file_descriptor == -1){  //error checking open
            perror("Failed to open output file");
            return -1;
        }
        redirect_indices[redirect_op_total] = redirection_index;    // add the index of ">>" argument to redirection_indices array  
        redirect_op_total++;                                        // increment the total number of redirection arguments
        redirect_indices[redirect_op_total] = redirection_index + 1;// add the index of the redirected file 
        redirect_op_total++;                                        // increment the total number of redirection arguments
        if(arrow_redirect(">>", out_file_descriptor) == -1){        // redirects output & error checks
            printf("could not redirect & append output\n");
            return -1;
        }
        redirected_output = 1;                                      // indicate that output was redirected
    }


    if(!(redirected_output || redirected_input)){   // branch entered when there is no redirection
        for(i = 0; i < size; i++){
            args[i] = strvec_get(tokens, i);            // all arguments are passed in to the args array
        }
        args[i] = NULL;                                 // NULL terminator at end of array
    }
    else {                                          // branch entered when there is redirection
        cur_redir_idx = 0;                              // current index of redirection arguements array        
        for(i = 0; i < size; i++){
            if(i == redirect_indices[cur_redir_idx]){   // ensure that redirection arguements are not passed to exec
                cur_redir_idx++;
            }
            else{
                args[i - cur_redir_idx] =  strvec_get(tokens, i);
            }
        }
        args[i - cur_redir_idx] = NULL;             // NULL terminator at end of array
    }

    

    execvp(args[0], args);
    //the following is executed upon a failed call to exec - basically it cleans up redirection & prints error
    perror("exec");
    if(redirected_output){
        dup2(backup_output, STDOUT_FILENO);
        close(out_file_descriptor);
    }
    if(redirected_input){
        dup2(backup_input, STDIN_FILENO);
        close(in_file_descriptor);
    }
    return -1;

    // Not reachable after a successful exec(), but retain here to keep compiler happy
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground)
{

    int status;
    int index = atoi(strvec_get(tokens, 1));    // convert the index passed as a string to a int
    
    if(!(jobs->length > index) || (index < 0)) {              // error checks that the index passed in is acceptable
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }
    job_t *curr_job = job_list_get(jobs, index);
    pid_t curr_pid = curr_job->pid;

    if(is_foreground){                                  //branch entered if the process should be in the foreground - uses truthy value to enter branch
        if(tcsetpgrp(STDIN_FILENO, curr_pid) == -1){    //error checks & makes the curr_pid the foreground process
            perror("tcsetpgrp");
            return -1;
        }
        if (kill(curr_pid, SIGCONT) == -1) {            //error checking & sending the SIGCONT signal to a curr_pid
            perror("kill, in resume job");
            return -1;
        }
        if(waitpid(-1, &status, WUNTRACED) == -1){      //error checks & waits for any child process
            perror("waitpid");
            return -1;
        }
    
        if (!WIFSTOPPED(status)) {                      //branch entered if the process was not stopped
            if(job_list_remove(jobs, index) == -1) {    //error checks & removes the job from the joblist
                printf("Problem adding job to job list\n");
                return -1;
            }
        }
        if(tcsetpgrp(STDIN_FILENO, getpid()) == -1){    //error checks & makes the current process id the foreground process
            perror("tcsetpgrp");
            return -1;
        }
    }
    else{                                               //branch entered when process should be in the background
        if (kill(curr_pid, SIGCONT) == -1) {            //error checking & sending the SIGCONT signal to a curr_pid
            perror("kill, in resume job");
            return -1;
        }
        curr_job->status = JOB_BACKGROUND;              //sets the job status of the current job to the background
    }

    

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs)
{
    char* idx_token = strvec_get(tokens, 1);
    if(idx_token == NULL){                                  //error checks strvec_get
        printf("strvec_get\n");
        return -1;
    }
    int idx = atoi(idx_token);                              // converts the index from a string to an int
    if(idx < 0 || idx >= jobs->length){                     //boundary checking index
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }
    job_t *currJob;
    if ((currJob = job_list_get(jobs, idx)) == NULL){       //error checks & retrieves job from the index
        printf("job_list_remove\n");
        return -1;
    }
    if(currJob->status != JOB_BACKGROUND){                  // branch entered if the job is in the background
        fprintf(stderr, "Job index is for stopped process not background process\n");
        return -1;
    }
    pid_t thisJobPid = currJob->pid;                        //retrieves the process id of the job
    if (waitpid(thisJobPid, NULL, WUNTRACED) == -1){        //error checking wait & waiting for this job to finish
        perror("waitpid");
        return -1;
    }
    if(job_list_remove(jobs, idx) == -1){                   //error checking & removes job from the job list as it is done
        printf("job_list_remove\n");
        return -1;
    }
    return 0;
}

int await_all_background_jobs(job_list_t *jobs){
    job_t *currJob = jobs->head;                                //sets currJob to the first job in the job list
    pid_t thisJobPid;
    while(currJob != NULL){         
        if(currJob->status == JOB_BACKGROUND){                  //enters branch when the current job is in the background
            thisJobPid = currJob->pid;
            if (waitpid(thisJobPid, NULL, WUNTRACED) == -1){    //error checking & waits for the job to finish
                perror("waitpid");
                return -1;
            }
        }
        currJob = currJob->next;                                //sets the currJob to the next job
    }
    job_list_remove_by_status(jobs, JOB_BACKGROUND);            //removes all background jobs
    return 0;
}
