#ifndef _ELF_H_
#define _ELF_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_CODE 4096
#define BASE_ADDR 0x400000
#define ENTRY_OFFSET 0x78

void writeELF(const char *filename, uint8_t *code, size_t code_size);

#endif