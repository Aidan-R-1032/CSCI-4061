#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void f(int limit) {
    for (int i = 1; i <= limit; i++) {
        printf("%d\n", i);
        sleep(1);
    }
}

int main() {
    // Create a signal set with SIGINT as only member
    sigset_t set;
    if (sigemptyset(&set) == -1 || sigaddset(&set, SIGINT) == -1) {
        perror("Failed to set up signal set");
        return 1;
    }

    // Start blocking SIGINT
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }
    f(20);
    // Stop blocking SIGINT
    if (sigprocmask(SIG_UNBLOCK, &set, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    printf("All done!\n");
    return 0;
}
