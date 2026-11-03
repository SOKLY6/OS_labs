#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/wait.h>

static void child_event(int sig) {
    (void)sig;
}

static int send_line(int fd, const char *str, size_t size) {
    off_t length = (off_t)size;

    if (length < 0 || (size_t)length != size) {
        errno = EOVERFLOW;
        perror("line size");
        return -1;
    }
    if (ftruncate(fd, length) == -1) {
        perror("ftruncate");
        return -1;
    }

    if (size == 0)
        return 0;

    char *mem = mmap(NULL, size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);

    if (mem == MAP_FAILED) {
        perror("mmap parent");
        return -1;
    }
    memcpy(mem, str, size);

    if (munmap(mem, size) == -1) {
        perror("munmap parent");
        return -1;
    }

    return 0;
}

int main(void) {
    char *names[2] = {NULL, NULL};
    char *str = NULL;
    size_t capacity = 0;
    int files[2] = {-1, -1};
    pid_t children[2] = {-1, -1};
    int result = 0;
    sigset_t signals, replies;

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = child_event;
    action.sa_flags = SA_NOCLDSTOP;

    if (sigemptyset(&action.sa_mask) == -1 ||
        sigaction(SIGCHLD, &action, NULL) == -1) {
        perror("sigaction");
        return 1;
    }
    if (sigemptyset(&signals) == -1 ||
        sigaddset(&signals, SIGUSR1) == -1 ||
        sigaddset(&signals, SIGUSR2) == -1 ||
        sigaddset(&signals, SIGCHLD) == -1 ||
        sigemptyset(&replies) == -1 ||
        sigaddset(&replies, SIGUSR2) == -1 ||
        sigaddset(&replies, SIGCHLD) == -1 ||
        sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("signal setup");
        return 1;
    }
    for (int fd = 0; fd < 3; ++fd) {
        if (fcntl(fd, F_GETFD) == -1) {
            perror("standard descriptor");
            return 1;
        }
    }
    for (int i = 0; i < 2 && result == 0; i++) {
        size_t name_capacity = 0;
        ssize_t len = getline(&names[i], &name_capacity, stdin);

        if (len == -1) {
            if (feof(stdin))
                fprintf(stderr, "Missing file name\n");
            else
                perror("getline");

            result = 1;
            break;
        }
        if (len > 0 && names[i][len - 1] == '\n')
            names[i][--len] = '\0';

        if (len == 0) {
            fprintf(stderr, "Empty file name\n");
            result = 1;
        }
    }
    if (result == 0 && strcmp(names[0], names[1]) == 0) {
        fprintf(stderr, "File names must be different\n");
        result = 1;
    }
    for (int i = 0; i < 2 && result == 0; i++) {
        char path[] = "/tmp/lab4-XXXXXX";
        files[i] = mkstemp(path);

        if (files[i] == -1) {
            perror("mkstemp");
            result = 1;
        } else if (unlink(path) == -1) {
            perror("unlink");
            result = 1;
        }
    }
    for (int i = 0; i < 2 && result == 0; i++) {
        children[i] = fork();

        if (children[i] == -1) {
            perror("fork");
            result = 1;
            break;
        }
        if (children[i] == 0) {
            int fd = open(names[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd == -1) {
                perror("open");
                _exit(1);
            }
            if (dup2(files[i], STDIN_FILENO) == -1 ||
                dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }
            int unused[] = {files[0], files[1], fd};

            for (int j = 0; j < 3; j++) {
                if (close(unused[j]) == -1) {
                    perror("close");
                    _exit(1);
                }
            }
            if (dprintf(STDERR_FILENO,
                        "Child PID=%ld, parent PID=%ld\n",
                        (long)getpid(), (long)getppid()) < 0) {
                perror("dprintf");
                _exit(1);
            }
            execl("./child", "child", (char *)NULL);
            perror("execl");
            _exit(1);
        }
    }
    int input_child = 0;

    while (result == 0) {
        ssize_t len = getline(&str, &capacity, stdin);

        if (len == -1) {
            if (!feof(stdin)) {
                perror("getline");
                result = 1;
            }
            break;
        }
        if (send_line(files[input_child], str, (size_t)len + 1) == -1) {
            result = 1;
            break;
        }
        if (kill(children[input_child], SIGUSR1) == -1) {
            perror("kill DATA");
            result = 1;
            break;
        }
        int sig;
        int error = sigwait(&replies, &sig);

        if (error != 0) {
            errno = error;
            perror("sigwait");
            result = 1;
        } else if (sig == SIGCHLD) {
            fprintf(stderr, "Child exited before completion\n");
            result = 1;
        }
        input_child = !input_child;
    }
    for (int i = 0; i < 2 && result == 0; i++) {
        if (send_line(files[i], NULL, 0) == -1) {
            result = 1;
        } else if (kill(children[i], SIGUSR1) == -1) {
            perror("kill EOF");
            result = 1;
        }
    }
    if (result != 0) {
        for (int i = 0; i < 2; i++) {
            if (children[i] > 0 &&
                kill(children[i], SIGKILL) == -1 &&
                errno != ESRCH) {
                perror("kill cleanup");
            }
        }
    }
    for (int i = 0; i < 2; i++) {
        if (children[i] > 0) {
            int status;
            pid_t waited;

            do {
                waited = waitpid(children[i], &status, 0);
            } while (waited == -1 && errno == EINTR);

            if (waited == -1) {
                perror("waitpid");
                result = 1;
            } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                fprintf(stderr, "Child %ld failed\n", (long)children[i]);
                result = 1;
            }
        }
        if (files[i] != -1 && close(files[i]) == -1) {
            perror("close");
            result = 1;
        }
        free(names[i]);
    }
    free(str);
    return result;
}