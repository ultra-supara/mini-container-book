#include <stdio.h>
#include <unistd.h>

int main(void) {
    char* path = "/usr/bin/id";
    char* argv[] = {path, NULL};
    char* envp[] = {NULL};

    execve(path, argv, envp);
    perror("execve");
    return 1;
}

