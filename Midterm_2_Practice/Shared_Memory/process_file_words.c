#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_WORD_LEN 25

int count_word_lengths(const char *file_name, int *counts) {
  FILE *fd = fopen(file_name, "r");
  if (fd == NULL) {  // fopen error branch
    perror("fopen");
    return -1;
  }
  int length = 0;
  char file_char;
  int EOF_not_reached = 1;
  while (EOF_not_reached) {
    file_char = (char)fgetc(fd);
    while (file_char != EOF && file_char != ' ' && file_char != '\n') {
      length++;
      file_char = (char)fgetc(fd);
    }
    if (length > 0) {
      counts[length - 1] += 1;
    }
    length = 0;
    if (feof(fd)) {
      EOF_not_reached = 0;
    }
  }
  fclose(fd);
  return 0;
}

int main(int argc, char **argv) {
  if (argc == 1) {  // No files to consume, return immediately
    return 0;
  }
  int shmid = shmget(IPC_PRIVATE, sizeof(int) * MAX_WORD_LEN,
                     IPC_CREAT | S_IRUSR | S_IWUSR);  // obtain shared memory id
  int *shm_addr = shmat(shmid, NULL, 0);              // set up shared memory
  pid_t pid;
  int counts[MAX_WORD_LEN];
  for (int i = 0; i < MAX_WORD_LEN; i++) {
    counts[i] = 0;
  }
  for (int i = 1; i < argc; i++) {
    if ((pid = fork()) == -1) {
      perror("fork");
      exit(-1);
    }
    if (pid == 0) {
      count_word_lengths(argv[i], counts);
      for (int i = 0; i < MAX_WORD_LEN; i++) {
        shm_addr[i] += counts[i];  // shared memory access
        counts[i] = 0;
      }
      shmdt(shm_addr);  // detach from shared memory
      exit(0);
    }
  }
  int status;
  int return_val = 0;
  for (int i = 1; i < argc; i++) {
    wait(&status);
    if (WEXITSTATUS(status)) {
      return_val = -1;
    }
  }
  if (return_val == 0) {
    for (int i = 0; i < MAX_WORD_LEN; i++) {
      printf("%d-letter words: %d\n", i + 1, shm_addr[i]);
    }
  }
  shmctl(shmid, IPC_RMID, NULL);  // final deallocation
  return return_val;
}