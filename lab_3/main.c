#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "tim_sort.h"

static void *worker(void *arg) {
    Task *task = arg;

    long tid = syscall(SYS_gettid);

    if (tid == -1) {
        task->error = errno;
        return NULL;
    }

    task->tid = (uint64_t)tid;

    tim_sort(task->array, task->buffer, task->size);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 4 || strcmp(argv[2], "-t") != 0) {
        fprintf(
            stderr,
            "Usage: %s ARRAY_SIZE -t THREADS\n",
            argv[0]
        );
        return 1;
    }

    const char *arguments[2] = {argv[1], argv[3]};
    size_t values[2];

    for (int i = 0; i < 2; ++i) {
        char *end;
        errno = 0;

        unsigned long long value =
            strtoull(arguments[i], &end, 10);

        unsigned long long limit =
            i == 0 ? INT_MAX : 1024;

        if (
            errno ||
            arguments[i][0] < '0' ||
            arguments[i][0] > '9' ||
            *end ||
            value == 0 ||
            value > limit ||
            (i == 0 && value > SIZE_MAX / sizeof(int))
        ) {
            fprintf(
                stderr,
                "Invalid argument: %s\n",
                arguments[i]
            );
            return 1;
        }

        values[i] = (size_t)value;
    }

    size_t n = values[0];
    size_t p = values[1];

    if (p > n)
        p = n;

    int result = 1;

    int *array = malloc(n * sizeof(int));
    int *buffer = malloc(n * sizeof(int));

    Task *tasks = malloc(p * sizeof(Task));
    pthread_t *threads = malloc(p * sizeof(pthread_t));

    do {
        if (!array || !buffer || !tasks || !threads) {
            perror("malloc");
            break;
        }

        uint32_t state = 42;

        for (size_t i = 0; i < n; ++i) {
            state =
                state * UINT32_C(1664525)
                + UINT32_C(1013904223);

            array[i] = (int)(state % 1000000);
        }

        size_t offset = 0;

        for (size_t i = 0; i < p; ++i) {
            size_t length = n / p + (i < n % p);

            tasks[i] = (Task){
                array + offset,
                buffer + offset,
                length,
                0,
                0
            };

            offset += length;
        }

        struct timespec start;
        struct timespec finish;

        if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
            perror("clock_gettime");
            break;
        }

        size_t created = 0;
        int failed = 0;

        for (size_t i = 1; i < p; ++i) {
            int error = pthread_create(
                &threads[created],
                NULL,
                worker,
                &tasks[i]
            );

            if (error) {
                fprintf(
                    stderr,
                    "pthread_create: %s\n",
                    strerror(error)
                );

                failed = 1;
                break;
            }

            ++created;
        }

        if (!failed)
            worker(&tasks[0]);

        for (size_t i = 0; i < created; ++i) {
            int error = pthread_join(threads[i], NULL);

            if (error) {
                fprintf(
                    stderr,
                    "pthread_join: %s\n",
                    strerror(error)
                );

                exit(EXIT_FAILURE);
            }
        }

        for (size_t i = 0; i < p; ++i) {
            if (tasks[i].error) {
                fprintf(
                    stderr,
                    "thread ID: %s\n",
                    strerror(tasks[i].error)
                );

                failed = 1;
            }
        }

        if (failed)
            break;

        for (size_t width = 1; width < p; width *= 2) {
            for (
                size_t i = 0;
                i + width < p;
                i += 2 * width
            ) {
                size_t last =
                    i + 2 * width < p ? i + 2 * width : p;

                size_t lo =
                    (size_t)(tasks[i].array - array);

                size_t mid =
                    (size_t)(tasks[i + width].array - array);

                size_t hi =
                    (size_t)(tasks[last - 1].array - array)
                    + tasks[last - 1].size;

                merge_runs(array, buffer, lo, mid, hi);
            }
        }

        if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1) {
            perror("clock_gettime");
            break;
        }

        for (size_t i = 1; i < n; ++i) {
            if (array[i - 1] > array[i]) {
                fprintf(
                    stderr,
                    "Sorting error at index %zu\n",
                    i
                );

                failed = 1;
                break;
            }
        }

        if (failed)
            break;

        double seconds =
            (double)(finish.tv_sec - start.tv_sec)
            + (finish.tv_nsec - start.tv_nsec) / 1e9;

        printf(
            "N=%zu threads=%zu time=%.6f s sorted=yes\n",
            n,
            p,
            seconds
        );

        for (size_t i = 0; i < p; ++i) {
            printf(
                "PID=%ld TID=%llu elements=%zu\n",
                (long)getpid(),
                (unsigned long long)tasks[i].tid,
                tasks[i].size
            );
        }

        if (n <= 30) {
            for (size_t i = 0; i < n; ++i)
                printf("%d ", array[i]);

            putchar('\n');
        }

        if (fflush(stdout) == EOF) {
            perror("stdout");
            break;
        }

        result = 0;

    } while (0);

    free(threads);
    free(tasks);
    free(buffer);
    free(array);

    return result;
}