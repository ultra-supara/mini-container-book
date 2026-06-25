#define _GNU_SOURCE
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define SUDOERS_PATH "/etc/e_sudoers"

static void print_error(const char* message) {
    printf("error: %s: %s\n", message, strerror(errno));
}

static int username_to_uid(const char* username, uid_t* uid) {
    struct passwd* pwd;

    errno = 0;
    pwd = getpwnam(username);
    if (pwd == NULL) {
        if (errno != 0) {
            print_error("getpwnam");
        } else {
            printf("warn: no matching user for username %s\n", username);
        }
        return -1;
    }

    *uid = pwd->pw_uid;
    return 0;
}

static int match_sudoers(uid_t uid) {
    FILE* file = fopen(SUDOERS_PATH, "r");
    char buffer[256];
    int ret = -1;

    if (file == NULL) {
        print_error("fopen");
        return -1;
    }

    while (fscanf(file, "%255s", buffer) == 1) {
        uid_t entry_uid;
        if (username_to_uid(buffer, &entry_uid) != 0) {
            continue;
        }
        if (entry_uid == uid) {
            ret = 0;
            break;
        }
    }

    if (ferror(file)) {
        print_error("fscanf");
        ret = -1;
    }

    fclose(file);
    return ret;
}

int main(int argc, char** argv) {
    uid_t ruid;
    uid_t euid;
    uid_t suid;
    uid_t target_uid = 0;
    int command_index = 1;

    if (getresuid(&ruid, &euid, &suid) != 0) {
        print_error("getresuid");
        return 1;
    }

    if (match_sudoers(ruid) != 0) {
        printf("error: no matching entry found in sudoers file %s\n", SUDOERS_PATH);
        return 1;
    }

    if (argc >= 4 && strcmp(argv[1], "-u") == 0) {
        if (username_to_uid(argv[2], &target_uid) != 0) {
            printf("error: specified user not found: -u %s\n", argv[2]);
            return 1;
        }
        command_index = 3;
    } else if (argc < 2) {
        printf("error: no command specified\n");
        return 1;
    }

    if (setresuid(target_uid, target_uid, target_uid) != 0) {
        print_error("setresuid");
        return 1;
    }

    execvp(argv[command_index], &argv[command_index]);
    print_error("execvp");
    return 1;
}
