#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return 1;
    }

    if (pid == 0) {
        const char* message = "child\n";
        close(pipefd[0]);
        write(pipefd[1], message, strlen(message));
        close(pipefd[1]);
        return 0;
    }

    close(pipefd[1]);
    {
        char buffer[1024];
        ssize_t size = read(pipefd[0], buffer, sizeof(buffer));
        if (size == -1) {
            perror("read");
            close(pipefd[0]);
            return 1;
        }
        write(STDOUT_FILENO, buffer, (size_t)size);
    }
    close(pipefd[0]);

    if (waitpid(pid, NULL, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    return 0;
}

