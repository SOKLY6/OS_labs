#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

int main(void) {
    char *names[2] = {NULL, NULL};
    char *str = NULL;
    size_t capacity = 0;
    int pipes[2][2] = {{-1, -1}, {-1, -1}};
    pid_t children[2] = {-1, -1};
    int result = 0;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return 1;
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
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
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

            if (dup2(pipes[i][0], STDIN_FILENO) == -1 ||
                dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }

            int unused[] = {
                pipes[0][0], pipes[0][1],
                pipes[1][0], pipes[1][1], fd
            };

            for (int j = 0; j < 5; j++) {
                if (unused[j] != STDIN_FILENO &&
                    unused[j] != STDOUT_FILENO &&
                    close(unused[j]) == -1) {
                    perror("close");
                    _exit(1);
                }
            }

            if (dprintf(STDERR_FILENO, "Child PID=%ld, parent PID=%ld\n",
                        (long)getpid(), (long)getppid()) < 0)
                _exit(1);

            execl("./child", "child", (char *)NULL);
            perror("execl");
            _exit(1);
        }
    }

    for (int i = 0; i < 2; i++) {
        if (pipes[i][0] != -1) {
            if (close(pipes[i][0]) == -1) {
                perror("close");
                result = 1;
            }

            pipes[i][0] = -1;
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

        size_t sent = 0;

        while (sent < (size_t)len) {
            size_t chunk = (size_t)len - sent;
            ssize_t n = write(pipes[input_child][1], str + sent, chunk);

            if (n == -1) {
                if (errno == EINTR)
                    continue;

                perror("write");
                result = 1;
                break;
            }

            if (n == 0) {
                fprintf(stderr, "Write made no progress\n");
                result = 1;
                break;
            }

            sent += (size_t)n;
        }

        input_child = !input_child;
    }

    for (int i = 0; i < 2; i++) {
        if (pipes[i][1] != -1 && close(pipes[i][1]) == -1) {
            perror("close");
            result = 1;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (children[i] <= 0)
            continue;

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

    free(names[0]);
    free(names[1]);
    free(str);

    return result;
}