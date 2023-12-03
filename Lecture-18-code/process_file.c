#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

long process_file(char* file_name){
    struct stat statbuf;
    if (stat(file_name, &statbuf) == -1){
        perror("stat");
        return -1;
    }
    return statbuf.st_size;
}

void* thread_func(void* arg){
    char* file_name = (char *) arg;
    long result = process_file(file_name);
    return (void*) result;
}

int process_files(char* file_names[], int n){
    pthread_t *threads = malloc(sizeof(pthread_t) * n);
    long *answers = malloc(n * sizeof(long));
    for(int i = 0; i < n; i++){
        if(pthread_create(threads + i, NULL, thread_func, file_names[i]) != 0){
            printf(stderr, "pthread_create: %s\n", stderror(error));
            free(threads);
            free(answers);
            return -1;
        }
    }
    for(int i = 0; i < n; i++){
        if(pthread_join(threads + i, (void **) answers + i) != 0){
            printf(stderr, "pthread_join: %s\n", stderror(error));
            free(threads);
            free(answers);
            return -1;
        }
    }
    free(threads);
    free(answers);
    return 0;
}
int main(int argc, char** argv) {
    return process_files(argv + 1, argc - 1);
}