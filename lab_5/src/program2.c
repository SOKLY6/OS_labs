#include <dlfcn.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    const char *paths[] = {"./lib1.so", "./lib2.so"};
    void *handle = NULL;
    float (*integral)(float, float, float) = NULL;
    int (*gcf)(int, int) = NULL;
    int library = 0, command, result = 1;
    const char *error;

    for (;;) {
        if (!handle) {
            handle = dlopen(paths[library], RTLD_NOW | RTLD_LOCAL);
            if (!handle) {
                fprintf(stderr, "dlopen: %s\n", dlerror());
                break;
            }

            dlerror();
            integral = (float (*)(float, float, float))dlsym(handle, "SinIntegral");
            error = dlerror();
            if (error) {
                fprintf(stderr, "dlsym: %s\n", error);
                break;
            }

            gcf = (int (*)(int, int))dlsym(handle, "GCF");
            error = dlerror();
            if (error) {
                fprintf(stderr, "dlsym: %s\n", error);
                break;
            }
        }

        int status = scanf("%d", &command);
        if (status == EOF && !ferror(stdin)) {
            result = 0;
            break;
        }
        if (status != 1)
            break;

        if (command == 0) {
            int closed = dlclose(handle);
            handle = NULL;
            if (closed != 0) {
                fprintf(stderr, "dlclose: %s\n", dlerror());
                break;
            }
            library = !library;
        } else if (command == 1) {
            float A, B, e;
            if (scanf("%f%f%f", &A, &B, &e) != 3)
                break;

            float value = integral(A, B, e);
            if (!isfinite(value))
                break;

            printf("%g\n", value);
        } else if (command == 2) {
            int A, B;
            if (scanf("%d%d", &A, &B) != 2 || A <= 0 || B <= 0)
                break;

            printf("%d\n", gcf(A, B));
        } else {
            break;
        }
    }

    if (handle && dlclose(handle) != 0) {
        fprintf(stderr, "dlclose: %s\n", dlerror());
        result = 1;
    }
    if (result)
        fprintf(stderr, "Program failed or invalid input\n");

    return result;
}