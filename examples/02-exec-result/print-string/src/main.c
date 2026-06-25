#include <stddef.h>
#include <unistd.h>

int print_string(const char* const str) {
    size_t len = 0;
    ssize_t written;

    if (str == NULL) {
        return -1;
    }

    while (str[len] != '\0') {
        len++;
    }

    written = write(STDOUT_FILENO, str, len);
    if (written < 0 || (size_t)written != len) {
        return -1;
    }

    written = write(STDOUT_FILENO, "\n", 1);
    if (written != 1) {
        return -1;
    }

    return (int)len + 1;
}

int main(void) {
    int ret = print_string("Hello, World!");
    return ret < 0 ? 1 : 0;
}

