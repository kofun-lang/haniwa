#include "haniwa/haniwa.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("haniwa " HANIWA_VERSION);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--smoke-test") == 0) {
        return haniwa_run(1);
    }
    if (argc != 1) {
        fputs("usage: haniwa [--version|--smoke-test]\n", stderr);
        return 64;
    }
    return haniwa_run(0);
}
