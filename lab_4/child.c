#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main(void) {
    sigset_t signals;
    int result = 0;
    pid_t parent = getppid();

    if (sigemptyset(&signals) == -1 ||
        sigaddset(&signals, SIGUSR1) == -1 ||
        sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("signal setup");
        result = 1;
    }
    while (result == 0) {
        int sig;
        int error = sigwait(&signals, &sig);

        if (error != 0) {
            errno = error;
            perror("sigwait");
            result = 1;
            break;
        }
        struct stat info;

        if (fstat(STDIN_FILENO, &info) == -1) {
            perror("fstat");
            result = 1;
            break;
        }
        if (info.st_size == 0)
            break;

        if (info.st_size < 0 || (uintmax_t)info.st_size > SIZE_MAX) {
            fprintf(stderr, "Invalid mapping size\n");
            result = 1;
            break;
        }
        size_t size = (size_t)info.st_size;
        char *str = mmap(NULL, size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE, STDIN_FILENO, 0);

        if (str == MAP_FAILED) {
            perror("mmap child");
            result = 1;
            break;
        }
        if (str[size - 1] != '\0') {
            fprintf(stderr, "Missing string terminator\n");
            result = 1;
        } else {
            size_t len = strlen(str);

            if (len > 0 && str[len - 1] == '\n')
                len--;

            for (size_t i = 0; i < len / 2; i++) {
                char temp = str[i];
                str[i] = str[len - 1 - i];
                str[len - 1 - i] = temp;
            }
            if (fputs(str, stdout) == EOF || fflush(stdout) == EOF) {
                perror("output");
                result = 1;
            }
        }
        if (munmap(str, size) == -1) {
            perror("munmap child");
            result = 1;
        }
        if (result == 0 && kill(parent, SIGUSR2) == -1) {
            perror("kill ACK");
            result = 1;
        }
    }
    if (fclose(stdout) == EOF) {
        perror("fclose");
        result = 1;
    }
    if (close(STDIN_FILENO) == -1) {
        perror("close");
        result = 1;
    }
    return result;
}