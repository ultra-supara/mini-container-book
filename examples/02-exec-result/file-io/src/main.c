#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    const char* path = "file.txt";
    const char* content = "Hello, file I/O!\n";
    char buffer[1024];
    ssize_t size;
    int fd;

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open for write");
        return 1;
    }

    size = write(fd, content, strlen(content));
    if (size == -1 || (size_t)size != strlen(content)) {
        perror("write");
        close(fd);
        return 1;
    }

    if (close(fd) == -1) {
        perror("close after write");
        return 1;
    }

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open for read");
        return 1;
    }

    size = read(fd, buffer, sizeof(buffer) - 1);
    if (size == -1) {
        perror("read");
        close(fd);
        return 1;
    }

    buffer[size] = '\0';
    printf("Read content: %s", buffer);

    if (close(fd) == -1) {
        perror("close after read");
        return 1;
    }

    return 0;
}

