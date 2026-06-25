#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int child_func(void* arg) {
    (void)arg;
    execlp("ip", "ip", "link", "show", NULL);
    perror("execlp ip");
    return 1;
}

int main(void) {
    const size_t stack_size = 1024 * 1024;
    void* stack;
    pid_t pid;

    stack = malloc(stack_size);
    if (stack == NULL) {
        perror("malloc");
        return 1;
    }

    pid = clone(child_func, (char*)stack + stack_size, SIGCHLD | CLONE_NEWNET, NULL);
    if (pid == -1) {
        perror("clone");
        free(stack);
        return 1;
    }

    if (waitpid(pid, NULL, 0) == -1) {
        perror("waitpid");
        free(stack);
        return 1;
    }

    free(stack);
    return 0;
}

