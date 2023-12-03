// wc_signal.c: Counts lines/words/chars like wc but with graceful
// shutdown/reporting of results when SIGINT is received (e.g., from
// (Ctrl-c in a terminal).

#include <ctype.h> // provides isspace() and other char type funcs
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int keep_going = 1; // control variable to continue loop

// TODO: Add a signal handling function which will cause main loop to end

void sig_handler(int signal){
    keep_going = 0;
}

int main(int argc, char *argv[]) {
    // TODO: add struct/calls to sigaction() to handle SIGINT and shut
    // down "gracefully". Use sigaction() and associated structs. Ensure
    // that the flag SA_RESTART is set as well to ensure system calls
    // are automatically restarted.

    struct sigaction s_act;
    s_act.sa_flags = SA_RESTART;
    s_act.sa_handler = sig_handler;
    if (sigemptyset(&s_act.sa_mask) == -1) {
        perror("failed to clear set of blocked signals\n");
    }
    if(sigaction(SIGINT, &s_act, NULL) == -1){
        perror("could not set up signal handler\n");
    }

    int num_words = 0;
    int num_lines = 0;
    int num_chars = 0;

    char last = ' '; // Pretend last character is a space to start

    while (keep_going) {
        int cur_char = fgetc(stdin);

        if (cur_char == EOF) {
            keep_going = 0;
            if (last != '\n') {
                // Input ends without newline, so we add to line count
                num_lines++;
            }
        } else {
            num_chars++;
            if (cur_char == '\n') {
                num_lines++;
            }
            if (!isspace(cur_char) && isspace(last)) {
                // We've hit start of a new word
                num_words++;
            }
        }

        last = cur_char;
    }

    printf("\n"); // Extra newline in case of keyboard signal
    printf("%d lines, %d words, %d chars\n", num_lines, num_words, num_chars);
    return 0;
}
