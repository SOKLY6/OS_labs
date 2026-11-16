#include "functions.h"
#include <math.h>
#include <stdio.h>

int main(void) {
    int command, result = 1;

    for (;;) {
        int status = scanf("%d", &command);
        if (status == EOF && !ferror(stdin)) {
            result = 0;
            break;
        }
        if (status != 1)
            break;

        if (command == 1) {
            float A, B, e;
            if (scanf("%f%f%f", &A, &B, &e) != 3)
                break;

            float value = SinIntegral(A, B, e);
            if (!isfinite(value))
                break;

            printf("%g\n", value);
        } else if (command == 2) {
            int A, B;
            if (scanf("%d%d", &A, &B) != 2 || A <= 0 || B <= 0)
                break;

            printf("%d\n", GCF(A, B));
        } else {
            break;
        }
    }

    if (result)
        fprintf(stderr, "Invalid input\n");

    return result;
}