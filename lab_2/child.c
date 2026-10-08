#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char *str = NULL;
    size_t capacity = 0;
    size_t len = 0;
    while (getline(&str, &capacity, stdin) != -1) {
        len = strlen(str);
        if (len > 0 && str[len - 1] == '\n') {
            len--;
        }
        for (size_t i = 0; i < len / 2; i++) {
            char temp = str[i];
            str[i] = str[len - 1 - i];
            str[len - 1 - i] = temp;
        }
        if (fputs(str, stdout) == EOF) {
            perror("fputs");
            free(str);
            return 1;
        }
    }
    if (!feof(stdin)) {
        perror("getline");
        free(str);
        return 1;
    }
    if (fflush(stdout) == EOF) {
        perror("fflush");
        free(str);
        return 1;
    }
    free(str);
    return 0;
}