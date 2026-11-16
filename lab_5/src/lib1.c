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

        sum += sin((x + next) / 2) * (next - x);
        x = next;
    }

    return (float)sum;
}

int GCF(int A, int B) {
    while (B) {
        int r = A % B;
        A = B;
        B = r;
    }
    return A;
}