#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#define MAX_CODE_SIZE 4096
#define MAX_VARS 128
#define MAX_VAR_NAME 32
#define MAX_VAR_VALUE 256

// 平台偵測
#if defined(__aarch64__) || defined(__AARCH64EL__)
    #define TARGET_AARCH64
#elif defined(__x86_64__) || defined(_M_X64)
    #define TARGET_X86_64
#else
    #error "Unsupported architecture platforms (Needs x86_64 or aarch64)"
#endif

// ELF Header Structures
struct Elf64_Ehdr
{
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
};

struct Elf64_Phdr
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

// ELF 寫出
void WRITE_ELF(const char *filename, uint8_t *code, size_t code_len)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        perror("fopen");
        exit(1);
    }
    struct Elf64_Ehdr eh = {0};
    struct Elf64_Phdr ph = {0};
    // ELF header
    memcpy(eh.e_ident, "\x7f" "ELF", 4);
    eh.e_ident[4] = 2; // 64-bit
    eh.e_ident[5] = 1; // little endian
    eh.e_ident[6] = 1; // version
    eh.e_type = 2;     // executable
    
    #ifdef TARGET_AARCH64
        eh.e_machine = 0xB7;
    #elif defined(TARGET_X86_64)
        eh.e_machine = 0x3E;
    #endif
    
    eh.e_version = 1;
    eh.e_entry = 0x400000 + sizeof(eh) + sizeof(ph);
    eh.e_phoff = sizeof(eh);
    eh.e_ehsize = sizeof(eh);
    eh.e_phentsize = sizeof(ph);
    eh.e_phnum = 1;
    
    // Program header
    ph.p_type = 1; // LOAD
    ph.p_flags = 5 | 1; // R + X
    ph.p_offset = 0;
    ph.p_vaddr = 0x400000;
    ph.p_paddr = 0x400000;
    ph.p_filesz = sizeof(eh) + sizeof(ph) + code_len;
    ph.p_memsz = ph.p_filesz;
    ph.p_align = 0x1000;
    
    fwrite(&eh, sizeof(eh), 1, fp);
    fwrite(&ph, sizeof(ph), 1, fp);
    fwrite(code, code_len, 1, fp);
    fclose(fp);
}

// sys_write 系統呼叫機器碼產生器
int GENERATE_SYSCALL(uint8_t *dst, const char *str, int newline)
{
    size_t len = strlen(str);
    if (newline) len++;
    
    #ifdef TARGET_X86_64
    // Linux x86_64 syscall: write(1, str, len)
    // mov rax, 1
    // mov rdi, 1
    // mov rsi, addr
    // mov rdx, len
    // syscall
    uint8_t code[] = {
        0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
        0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00, // mov rdi, 1
        0x48, 0xbe, 0,0,0,0,0,0,0,0,              // mov rsi, addr
        0x48, 0xc7, 0xc2, 0,0,0,0,                // mov rdx, len
        0x0f, 0x05                                // syscall
    };
    memcpy(dst, code, sizeof(code));
    (uint64_t)(dst + 14) = (uint64_t)(0x600000);  // rsi = .data addr
    (uint32_t)(dst + 24) = (uint32_t)len;
    memcpy((void*)(dst + 32), str, len);
    if (newline) dst[32 + len - 1] = '\n';
    return sizeof(code) + len;
    #elif defined(TARGET_AARCH64)
    // mov x8, #64 (sys_write)
    // mov x0, #1
    // adr x1, str
    // mov x2, #len
    // svc #0
    uint8_t code[] = {
        0xd2,0x80,0x08,0x08,  // mov x8, #64
        0xd2,0x80,0x00,0x00,  // mov x0, #1
        0x10,0x00,0x00,0x58,  // ldr x1, #offset
        0xd2,0x80,0x00,0x00,  // mov x2, #len
        0x01,0x00,0x00,0xd4   // svc #0
    };
    memcpy(dst, code, sizeof(code));
    dst[12] = 0x10; // ldr x1
    dst[16] = 0xd2;
    dst[17] = 0x80;
    dst[18] = 0x00;
    dst[19] = 0x00; // mov x2, #len (低位元)
    *((uint32_t*)(dst + 16)) = 0xd2800000 | ((uint32_t)(len & 0xFFFF) << 5); // mov x2, len
    memcpy((void*)(dst + 20), str, len);
    if (newline) dst[20 + len - 1] = '\n';
    return sizeof(code) + len;
    #endif
}

// 變量
typedef struct
{
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
} Var;
Var vars[MAX_VARS];
int var_count = 0;

const char *get_var(const char *name)
{
    for (int i = 0; i < var_count; ++i)
        if (strcmp(vars[i].name, name) == 0)
            return vars[i].value;
    return "<undefined>";
}

void set_var(const char *name, const char *value)
{
    for (int i = 0; i < var_count; ++i)
        if (strcmp(vars[i].name, name) == 0)
        {
            strncpy(vars[i].value, value, MAX_VAR_VALUE);
            return;
        }
    strncpy(vars[var_count].name, name, MAX_VAR_NAME);
    strncpy(vars[var_count].value, value, MAX_VAR_VALUE);
    var_count++;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        printf("Usage: %s <src.sc> -o <out.elf>\n", argv[0]);
        return 1;
    }
    
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("Cannot open source file.");
        return 1;
    }
    
    char line[1024];
    uint8_t code[MAX_CODE_SIZE];
    size_t code_len = 0;
    
    while (fgets(line, sizeof(line), fp))
    {
        char *trim = strtok(line, "\n");
        if (!trim) continue;
        // var = 'value';
        char varname[MAX_VAR_NAME], val[MAX_VAR_VALUE];
        if (sscanf(trim, "%31[^=]= '%255[^']';", varname, val) == 2)
        {
            set_var(varname, val);
            continue;
        }
        // out('str'); or outln('str');
        if (strncmp(trim, "out", 3) == 0)
        {
            int newline = strstr(trim, "outln") != NULL;
            char content[512];
            if (sscanf(trim, "out%*[^(']( '%[^']' );", content) == 1)
            {
                code_len += GENERATE_SYSCALL(code + code_len, content, newline);
                continue;
            } else if (sscanf(trim, "out%*[^(']( %[^);] );", varname) == 1)
            {
                const char *str = get_var(varname);
                code_len += GENERATE_SYSCALL(code + code_len, str, newline);
                continue;
            }
        }
    }
    fclose(fp);
    
    WRITE_ELF(argv[3], code, code_len);
    printf("Done：%s\n", argv[3]);
    return 0;
}