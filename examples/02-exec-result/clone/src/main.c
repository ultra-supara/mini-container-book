#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int child(void* arg) {
    const char* message = (const char*)arg;
    puts(message);
    fflush(stdout);
    return 0;
}

int main(void) {
    const size_t stack_size = 1024 * 1024;
    const char* message = "Hello, child process!";
    void* stack;
    pid_t child_pid;
    int child_status;

    stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    child_pid = clone(child, (char*)stack + stack_size, SIGCHLD, (void*)message);
    if (child_pid == -1) {
        perror("clone");
        munmap(stack, stack_size);
        return 1;
    }

    if (waitpid(child_pid, &child_status, 0) == -1) {
        perror("waitpid");
        munmap(stack, stack_size);
        return 1;
    }

    if (WIFEXITED(child_status)) {
        printf("Child process exited with status: %d\n", WEXITSTATUS(child_status));
    }

    munmap(stack, stack_size);
    return 0;
}

