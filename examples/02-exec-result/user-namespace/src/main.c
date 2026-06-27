#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void print_uid(const char* label) {
    printf("[%s]UID=%d\n", label, (int)getuid());
    fflush(stdout);
}

static int child(void* arg) {
    print_uid((const char*)arg);
    return 0;
}

int main(void) {
    const size_t stack_size = 1024 * 1024;
    void* stack;
    pid_t child_pid;

    print_uid("parent");

    stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    child_pid = clone(child, (char*)stack + stack_size, CLONE_NEWUSER | SIGCHLD, "child");
    if (child_pid == -1) {
        perror("clone");
        munmap(stack, stack_size);
        return 1;
    }

    if (waitpid(child_pid, NULL, 0) == -1) {
        perror("waitpid");
        munmap(stack, stack_size);
        return 1;
    }

    munmap(stack, stack_size);
    return 0;
}

