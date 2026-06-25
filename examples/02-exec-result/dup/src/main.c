#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    const char* message = "Hello from duplicated stdout\n";
    int new_fd = dup(STDOUT_FILENO);

    if (new_fd == -1) {
        perror("dup");
        return 1;
    }

    if (write(new_fd, message, strlen(message)) == -1) {
        perror("write");
        close(new_fd);
        return 1;
    }

    close(new_fd);
    return 0;
}

