#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"

// ELF Header结构定义
typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

void writeELF(const char *filename, uint8_t *code, size_t code_size) {
    uint8_t elf[MAX_CODE] = {0};

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elf;
    Elf64_Phdr *phdr = (Elf64_Phdr *)(elf + sizeof(Elf64_Ehdr));

    // ELF Header
    memset(ehdr, 0, sizeof(Elf64_Ehdr));
    ehdr->e_ident[0] = 0x7f;
    memcpy(&ehdr->e_ident[1], "ELF", 3);
    ehdr->e_ident[4] = 2; // 64-bit
    ehdr->e_ident[5] = 1; // little-endian
    ehdr->e_ident[6] = 1; // version
    ehdr->e_type = 2;
    ehdr->e_machine = 0x3E;
    ehdr->e_version = 1;
    ehdr->e_entry = BASE_ADDR + ENTRY_OFFSET;
    ehdr->e_phoff = sizeof(Elf64_Ehdr);
    ehdr->e_ehsize = sizeof(Elf64_Ehdr);
    ehdr->e_phentsize = sizeof(Elf64_Phdr);
    ehdr->e_phnum = 1;

    // Program Header
    memset(phdr, 0, sizeof(Elf64_Phdr));
    phdr->p_type = 1; // PT_LOAD
    phdr->p_flags = 5; // R-X
    phdr->p_offset = 0;
    phdr->p_vaddr = BASE_ADDR;
    phdr->p_paddr = BASE_ADDR;
    phdr->p_filesz = ENTRY_OFFSET + code_size;
    phdr->p_memsz = ENTRY_OFFSET + code_size;
    phdr->p_align = 0x200000;

    // 将代码复制到ELF文件的正确位置
    memcpy(elf + ENTRY_OFFSET, code, code_size);

    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    fwrite(elf, 1, ENTRY_OFFSET + code_size, f);
    fclose(f);
}