#ifndef TIM_SORT_H
#define TIM_SORT_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t start;
    size_t length;
} Run;

typedef struct {
    int *array;
    int *buffer;
    size_t size;
    uint64_t tid;
    int error;
} Task;

size_t build_run(
    int *a,
    size_t n,
    size_t start,
    size_t minrun
);

void merge_runs(
    int *a,
    int *tmp,
    size_t lo,
    size_t mid,
    size_t hi
);

void tim_sort(int *a, int *tmp, size_t n);

#endif