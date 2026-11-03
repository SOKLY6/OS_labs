#include "tim_sort.h"

#include <limits.h>
#include <string.h>

size_t build_run(
    int *a,
    size_t n,
    size_t start,
    size_t minrun
) {
    size_t end = start + 1;

    if (end < n && a[end] < a[start]) {
        while (end < n && a[end] < a[end - 1])
            ++end;

        for (
            size_t left = start, right = end - 1;
            left < right;
            ++left, --right
        ) {
            int value = a[left];
            a[left] = a[right];
            a[right] = value;
        }
    } else {
        while (end < n && a[end] >= a[end - 1])
            ++end;
    }

    size_t target =
        start + (n - start < minrun ? n - start : minrun);

    while (end < target) {
        int value = a[end];
        size_t left = start;
        size_t right = end;

        while (left < right) {
            size_t middle = left + (right - left) / 2;

            if (value < a[middle])
                right = middle;
            else
                left = middle + 1;
        }

        memmove(
            a + left + 1,
            a + left,
            (end - left) * sizeof(*a)
        );

        a[left] = value;
        ++end;
    }

    return end - start;
}


void merge_runs(
    int *a,
    int *tmp,
    size_t lo,
    size_t mid,
    size_t hi
) {
    if (lo == mid || mid == hi || a[mid - 1] <= a[mid])
        return;

    memcpy(
        tmp + lo,
        a + lo,
        (hi - lo) * sizeof(*a)
    );

    size_t i = lo;
    size_t j = mid;
    size_t k = lo;

    unsigned left_wins = 0;
    unsigned right_wins = 0;

    while (i < mid && j < hi) {
        if (tmp[i] <= tmp[j]) {
            a[k++] = tmp[i++];

            ++left_wins;
            right_wins = 0;
        } else {
            a[k++] = tmp[j++];

            ++right_wins;
            left_wins = 0;
        }

        if (
            (left_wins >= 7 || right_wins >= 7) &&
            i < mid &&
            j < hi
        ) {
            int from_left = left_wins >= 7;

            size_t base = from_left ? i : j;
            size_t limit = from_left ? mid : hi;

            int key = from_left ? tmp[j] : tmp[i];

            size_t left = base;
            size_t right = limit;
            size_t step = 1;

            while (step < limit - base) {
                size_t probe = base + step;

                if (
                    tmp[probe] < key ||
                    (from_left && tmp[probe] == key)
                ) {
                    left = probe + 1;

                    if (step > (limit - base) / 2)
                        break;

                    step *= 2;
                } else {
                    right = probe;
                    break;
                }
            }

            while (left < right) {
                size_t middle = left + (right - left) / 2;

                if (
                    tmp[middle] < key ||
                    (from_left && tmp[middle] == key)
                ) {
                    left = middle + 1;
                } else {
                    right = middle;
                }
            }

            memcpy(
                a + k,
                tmp + base,
                (left - base) * sizeof(*a)
            );

            k += left - base;

            if (from_left)
                i = left;
            else
                j = left;

            left_wins = 0;
            right_wins = 0;
        }
    }

    if (i < mid) {
        memcpy(
            a + k,
            tmp + i,
            (mid - i) * sizeof(*a)
        );
    } else if (j < hi) {
        memcpy(
            a + k,
            tmp + j,
            (hi - j) * sizeof(*a)
        );
    }
}


void tim_sort(int *a, int *tmp, size_t n) {
    if (n < 2)
        return;

    size_t minrun = n;
    size_t remainder = 0;

    while (minrun >= 64) {
        remainder |= minrun & 1;
        minrun >>= 1;
    }

    minrun += remainder;

    Run stack[2 * sizeof(size_t) * CHAR_BIT + 2];

    size_t count = 0;
    size_t position = 0;

    while (position < n || count > 1) {
        if (position < n) {
            size_t length = build_run(
                a,
                n,
                position,
                minrun
            );

            stack[count++] = (Run){position, length};
            position += length;
        }

        while (count > 1) {
            size_t s = count - 2;

            if (position == n) {
                if (
                    s > 0 &&
                    stack[s - 1].length < stack[s + 1].length
                ) {
                    --s;
                }
            } else if (
                (
                    s > 0 &&
                    stack[s - 1].length <=
                    stack[s].length + stack[s + 1].length
                )
                ||
                (
                    s > 1 &&
                    stack[s - 2].length <=
                    stack[s - 1].length + stack[s].length
                )
            ) {
                if (
                    stack[s - 1].length < stack[s + 1].length
                ) {
                    --s;
                }
            } else if (
                stack[s].length > stack[s + 1].length
            ) {
                break;
            }

            size_t lo = stack[s].start;
            size_t mid = stack[s + 1].start;
            size_t hi = mid + stack[s + 1].length;

            merge_runs(a, tmp, lo, mid, hi);

            stack[s].length += stack[s + 1].length;

            for (size_t q = s + 1; q + 1 < count; ++q)
                stack[q] = stack[q + 1];

            --count;
        }
    }
}