#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "elf.h"
#include "syscall.h"

int main(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[2], "-o") != 0) {
        fprintf(stderr, "Usage: %s source.sc -o output\n", argv[0]);
        return 1;
    }

    FILE *src = fopen(argv[1], "r");
    if (!src) {
        perror("Cannot open the output file.");
        return 1;
    }

    uint8_t code[MAX_CODE];
    size_t code_len = 0;

    char line[256];
    while (fgets(line, sizeof(line), src)) {
        char str[128];
        if (sscanf(line, "out('%[^']');", str) == 1) {
            code_len += generateSyscall(code + code_len, str, 0);
        } else if (sscanf(line, "outln('%[^']');", str) == 1) {
            code_len += generateSyscall(code + code_len, str, 1);
        } else if (sscanf(line, "out(\"%[^\"]\");", str) == 1) {
            code_len += generateSyscall(code + code_len, str, 0);
        } else if (sscanf(line, "outln(\"%[^\"]\");", str) == 1) {
            code_len += generateSyscall(code + code_len, str, 1);
        } else {
            fprintf(stderr, "Unknown command: %s", line);
        }
    }

    fclose(src);
    writeELF(argv[3], code, code_len);
    return 0;
}