#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)
#define HOST_IFACE "mc-host0"
#define CHILD_IFACE "mc-child0"
#define CONTAINER_IFACE "eth0"
#define HOST_ADDR_CIDR "10.200.0.1/24"
#define CHILD_ADDR_CIDR "10.200.0.2/24"
#define GATEWAY_ADDR "10.200.0.1"
#define SUBNET_CIDR "10.200.0.0/24"

struct container_config {
    const char* rootfs;
    char** command;
    const char* hostname;
    const char* nat_if;
    bool use_userns;
    bool use_network;
    bool mount_proc;
    int sync_pipe[2];
};

static void usage(FILE* out, const char* argv0) {
    fprintf(out,
            "usage: %s [OPTIONS] ROOTFS COMMAND [ARGS...]\n"
            "\n"
            "options:\n"
            "  --hostname NAME   set hostname inside the UTS namespace\n"
            "  --userns          map the current uid/gid to uid/gid 0 in a new user namespace\n"
            "  --network         create a veth pair and configure eth0 in the container\n"
            "  --nat-if IFACE    add iptables NAT rules through IFACE; implies --network\n"
            "  --mount-proc      mount procfs at /proc after chroot\n"
            "  --help            show this help\n",
            argv0);
}

static int parse_args(int argc, char** argv, struct container_config* config) {
    memset(config, 0, sizeof(*config));
    config->hostname = "mini-container";

    int index = 1;
    while (index < argc) {
        if (strcmp(argv[index], "--help") == 0) {
            usage(stdout, argv[0]);
            return 1;
        }
        if (strcmp(argv[index], "--hostname") == 0) {
            if (index + 1 >= argc) {
                fprintf(stderr, "--hostname requires a value\n");
                return -1;
            }
            config->hostname = argv[index + 1];
            index += 2;
            continue;
        }
        if (strcmp(argv[index], "--userns") == 0) {
            config->use_userns = true;
            index++;
            continue;
        }
        if (strcmp(argv[index], "--network") == 0) {
            config->use_network = true;
            index++;
            continue;
        }
        if (strcmp(argv[index], "--nat-if") == 0) {
            if (index + 1 >= argc) {
                fprintf(stderr, "--nat-if requires a value\n");
                return -1;
            }
            config->use_network = true;
            config->nat_if = argv[index + 1];
            index += 2;
            continue;
        }
        if (strcmp(argv[index], "--mount-proc") == 0) {
            config->mount_proc = true;
            index++;
            continue;
        }
        if (strcmp(argv[index], "--") == 0) {
            index++;
            break;
        }
        if (strncmp(argv[index], "--", 2) == 0) {
            fprintf(stderr, "unknown option: %s\n", argv[index]);
            return -1;
        }
        break;
    }

    if (argc - index < 2) {
        usage(stderr, argv[0]);
        return -1;
    }

    config->rootfs = argv[index];
    config->command = &argv[index + 1];
    return 0;
}

static int wait_for_child(pid_t pid, int* status) {
    for (;;) {
        if (waitpid(pid, status, 0) == pid) {
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }
        perror("waitpid");
        return -1;
    }
}

static int run_program(char* const argv[], bool allow_failure) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        if (allow_failure) {
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull != -1) {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                close(devnull);
            }
        }
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }

    int status = 0;
    if (wait_for_child(pid, &status) != 0) {
        return -1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }

    if (!allow_failure) {
        fprintf(stderr, "command failed:");
        for (size_t i = 0; argv[i] != NULL; i++) {
            fprintf(stderr, " %s", argv[i]);
        }
        fprintf(stderr, "\n");
    }

    return allow_failure ? 0 : -1;
}

static int write_file(const char* path, const char* data, bool required) {
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        if (required) {
            perror(path);
        }
        return -1;
    }

    size_t len = strlen(data);
    size_t offset = 0;
    while (offset < len) {
        ssize_t n = write(fd, data + offset, len - offset);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("write");
            close(fd);
            return -1;
        }
        offset += (size_t)n;
    }

    if (close(fd) != 0) {
        perror("close");
        return -1;
    }
    return 0;
}

static int setup_user_map(pid_t pid, uid_t host_uid, gid_t host_gid) {
    char path[256];
    char map[256];

    snprintf(path, sizeof(path), "/proc/%d/setgroups", pid);
    write_file(path, "deny\n", false);

    snprintf(path, sizeof(path), "/proc/%d/uid_map", pid);
    snprintf(map, sizeof(map), "0 %d 1\n", (int)host_uid);
    if (write_file(path, map, true) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "/proc/%d/gid_map", pid);
    snprintf(map, sizeof(map), "0 %d 1\n", (int)host_gid);
    if (write_file(path, map, true) != 0) {
        return -1;
    }

    return 0;
}

static int wait_parent_ready(int fd) {
    char c;
    for (;;) {
        ssize_t n = read(fd, &c, 1);
        if (n == 1) {
            return 0;
        }
        if (n == -1 && errno == EINTR) {
            continue;
        }
        if (n == 0) {
            fprintf(stderr, "parent closed sync pipe before container was ready\n");
        } else {
            perror("read sync");
        }
        return -1;
    }
}

static int notify_child_ready(int fd) {
    for (;;) {
        ssize_t n = write(fd, "1", 1);
        if (n == 1) {
            return 0;
        }
        if (n == -1 && errno == EINTR) {
            continue;
        }
        perror("write sync");
        return -1;
    }
}

static int setup_rootfs(const char* rootfs) {
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
        perror("mount private /");
        return -1;
    }
    if (chroot(rootfs) != 0) {
        perror("chroot");
        return -1;
    }
    if (chdir("/") != 0) {
        perror("chdir");
        return -1;
    }
    return 0;
}

static int mount_procfs(void) {
    if (mkdir("/proc", 0555) != 0 && errno != EEXIST) {
        perror("mkdir /proc");
        return -1;
    }
    if (mount("proc", "/proc", "proc", 0, "") != 0) {
        perror("mount proc");
        return -1;
    }
    return 0;
}

static int run_init(char** command) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        execvp(command[0], command);
        perror(command[0]);
        _exit(127);
    }

    int status = 0;
    int command_status = 1;
    for (;;) {
        pid_t waited = wait(&status);
        if (waited == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ECHILD) {
                break;
            }
            perror("wait");
            return 1;
        }

        if (waited == pid) {
            command_status = status;
            break;
        }
    }

    while (waitpid(-1, &status, WNOHANG) > 0) {
    }

    if (WIFEXITED(command_status)) {
        return WEXITSTATUS(command_status);
    }
    if (WIFSIGNALED(command_status)) {
        return 128 + WTERMSIG(command_status);
    }
    return 1;
}

static int setup_parent_network(pid_t child_pid) {
    char pid_buf[32];
    snprintf(pid_buf, sizeof(pid_buf), "%d", child_pid);

    char* const delete_old[] = {"ip", "link", "del", HOST_IFACE, NULL};
    run_program(delete_old, true);

    char* const add_veth[] = {
        "ip", "link", "add", HOST_IFACE, "type", "veth", "peer", "name", CHILD_IFACE, NULL,
    };
    if (run_program(add_veth, false) != 0) {
        return -1;
    }

    char* const move_child[] = {"ip", "link", "set", CHILD_IFACE, "netns", pid_buf, NULL};
    if (run_program(move_child, false) != 0) {
        return -1;
    }

    char* const addr_host[] = {"ip", "address", "add", HOST_ADDR_CIDR, "dev", HOST_IFACE, NULL};
    if (run_program(addr_host, false) != 0) {
        return -1;
    }

    char* const up_host[] = {"ip", "link", "set", HOST_IFACE, "up", NULL};
    if (run_program(up_host, false) != 0) {
        return -1;
    }

    return 0;
}

static int setup_child_network(void) {
    char* const up_loopback[] = {"ip", "link", "set", "lo", "up", NULL};
    if (run_program(up_loopback, false) != 0) {
        return -1;
    }

    char* const rename_child[] = {"ip", "link", "set", CHILD_IFACE, "name", CONTAINER_IFACE, NULL};
    if (run_program(rename_child, false) != 0) {
        return -1;
    }

    char* const addr_child[] = {
        "ip", "address", "add", CHILD_ADDR_CIDR, "dev", CONTAINER_IFACE, NULL,
    };
    if (run_program(addr_child, false) != 0) {
        return -1;
    }

    char* const up_child[] = {"ip", "link", "set", CONTAINER_IFACE, "up", NULL};
    if (run_program(up_child, false) != 0) {
        return -1;
    }

    char* const default_route[] = {"ip", "route", "add", "default", "via", GATEWAY_ADDR, NULL};
    if (run_program(default_route, false) != 0) {
        return -1;
    }

    return 0;
}

static void cleanup_parent_network(void) {
    char* const delete_veth[] = {"ip", "link", "del", HOST_IFACE, NULL};
    run_program(delete_veth, true);
}

static int setup_nat(const char* outbound_if) {
    if (write_file("/proc/sys/net/ipv4/ip_forward", "1\n", true) != 0) {
        return -1;
    }

    char* const nat_rule[] = {
        "iptables", "-t", "nat", "-A", "POSTROUTING", "-s", SUBNET_CIDR,
        "-o",       (char*)outbound_if, "-j", "MASQUERADE", NULL,
    };
    if (run_program(nat_rule, false) != 0) {
        return -1;
    }

    char* const forward_out[] = {
        "iptables", "-A", "FORWARD", "-i", HOST_IFACE, "-o", (char*)outbound_if, "-j", "ACCEPT", NULL,
    };
    if (run_program(forward_out, false) != 0) {
        return -1;
    }

    char* const forward_in[] = {
        "iptables", "-A", "FORWARD", "-i", (char*)outbound_if, "-o", HOST_IFACE,
        "-m",       "state", "--state", "RELATED,ESTABLISHED", "-j", "ACCEPT", NULL,
    };
    if (run_program(forward_in, false) != 0) {
        return -1;
    }

    return 0;
}

static void cleanup_nat(const char* outbound_if) {
    if (outbound_if == NULL) {
        return;
    }

    char* const nat_rule[] = {
        "iptables", "-t", "nat", "-D", "POSTROUTING", "-s", SUBNET_CIDR,
        "-o",       (char*)outbound_if, "-j", "MASQUERADE", NULL,
    };
    run_program(nat_rule, true);

    char* const forward_out[] = {
        "iptables", "-D", "FORWARD", "-i", HOST_IFACE, "-o", (char*)outbound_if, "-j", "ACCEPT", NULL,
    };
    run_program(forward_out, true);

    char* const forward_in[] = {
        "iptables", "-D", "FORWARD", "-i", (char*)outbound_if, "-o", HOST_IFACE,
        "-m",       "state", "--state", "RELATED,ESTABLISHED", "-j", "ACCEPT", NULL,
    };
    run_program(forward_in, true);
}

static int container_child(void* arg) {
    struct container_config* config = arg;
    close(config->sync_pipe[1]);

    if (wait_parent_ready(config->sync_pipe[0]) != 0) {
        return 1;
    }
    close(config->sync_pipe[0]);

    if (sethostname(config->hostname, strlen(config->hostname)) != 0) {
        perror("sethostname");
        return 1;
    }

    if (config->use_network && setup_child_network() != 0) {
        return 1;
    }

    if (setup_rootfs(config->rootfs) != 0) {
        return 1;
    }

    bool proc_mounted = false;
    if (config->mount_proc) {
        if (mount_procfs() != 0) {
            return 1;
        }
        proc_mounted = true;
    }

    int result = run_init(config->command);

    if (proc_mounted && umount2("/proc", MNT_DETACH) != 0) {
        perror("umount /proc");
    }

    return result;
}

static int start_container(struct container_config* config) {
    void* stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (pipe(config->sync_pipe) != 0) {
        perror("pipe");
        munmap(stack, STACK_SIZE);
        return 1;
    }

    int flags = SIGCHLD | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS;
    if (config->use_network) {
        flags |= CLONE_NEWNET;
    }
    if (config->use_userns) {
        flags |= CLONE_NEWUSER;
    }

    pid_t child = clone(container_child, (char*)stack + STACK_SIZE, flags, config);
    if (child == -1) {
        perror("clone");
        close(config->sync_pipe[0]);
        close(config->sync_pipe[1]);
        munmap(stack, STACK_SIZE);
        return 1;
    }

    close(config->sync_pipe[0]);

    int result = 1;
    if (config->use_userns &&
        setup_user_map(child, getuid(), getgid()) != 0) {
        goto fail;
    }

    if (config->use_network && setup_parent_network(child) != 0) {
        goto fail;
    }

    if (config->nat_if != NULL && setup_nat(config->nat_if) != 0) {
        goto fail;
    }

    if (notify_child_ready(config->sync_pipe[1]) != 0) {
        goto fail;
    }
    close(config->sync_pipe[1]);
    config->sync_pipe[1] = -1;

    int status = 0;
    if (wait_for_child(child, &status) != 0) {
        goto cleanup;
    }

    if (WIFEXITED(status)) {
        result = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result = 128 + WTERMSIG(status);
    }
    goto cleanup;

fail:
    kill(child, SIGKILL);
    if (config->sync_pipe[1] != -1) {
        close(config->sync_pipe[1]);
        config->sync_pipe[1] = -1;
    }
    waitpid(child, NULL, 0);

cleanup:
    cleanup_nat(config->nat_if);
    if (config->use_network) {
        cleanup_parent_network();
    }
    munmap(stack, STACK_SIZE);
    return result;
}

int main(int argc, char** argv) {
    struct container_config config;
    int parsed = parse_args(argc, argv, &config);
    if (parsed > 0) {
        return 0;
    }
    if (parsed < 0) {
        return 1;
    }
    return start_container(&config);
}
