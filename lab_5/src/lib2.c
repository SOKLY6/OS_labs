#include "functions.h"
#include <math.h>

float SinIntegral(float A, float B, float e) {
    if (!isfinite(A) || !isfinite(B) || !isfinite(e) || A > B || e <= 0)
        return NAN;

    double sum = 0;

    for (double x = A; x < B; ) {
        double next = fmin(x + e, B);
        if (next <= x)
            return NAN;

        sum += (sin(x) + sin(next)) * (next - x) / 2;
        x = next;
    }

    return (float)sum;
}

int GCF(int A, int B) {
    for (int d = A < B ? A : B; d > 0; --d)
        if (A % d == 0 && B % d == 0)
            return d;

    return 1;
}