#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    char input_buffer[1024];
    const char* prompt = "Input: ";
    ssize_t bytes_read;

    if (write(STDOUT_FILENO, prompt, strlen(prompt)) == -1) {
        perror("write prompt");
        return 1;
    }

    bytes_read = read(STDIN_FILENO, input_buffer, sizeof(input_buffer) - 1);
    if (bytes_read == -1) {
        perror("read");
        return 1;
    }

    input_buffer[bytes_read] = '\0';
    printf("Read from standard input: %s", input_buffer);
    return 0;
}

