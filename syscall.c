#include <string.h>
#include <stdio.h>
#include "syscall.h"
#include "elf.h"

// 生成 syscall 代码，并写入字符串 msg 到生成的代码中
size_t generateSyscall(uint8_t *buf, const char *msg, int newline)
{
    size_t len = strlen(msg);
    size_t total = 0;
    const char *final = msg;
    char tmp[256];
    if (newline)
    {
        snprintf(tmp, sizeof(tmp), "%s\n", msg);
        final = tmp;
        len += 1;
    }
    // 生成 syscall 的代码
    size_t msg_pos = ENTRY_OFFSET + 100 + total + 30;  // 預留 100 bytes for code
    size_t rel_offset = msg_pos - (ENTRY_OFFSET + 100 + total + 19); // rip+偏移 (after lea)
    uint8_t code[] = {
        0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00,       // mov rax, 1 (sys_write)
        0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00,       // mov rdi, 1 (stdout)
        0x48, 0x8d, 0x35, 0, 0, 0, 0,                   // lea rsi, [rip+offset]
        0x48, 0xc7, 0xc2, len, 0x00, 0x00, 0x00,        // mov rdx, len
        0x0f, 0x05                                      // syscall
    };
    // 填入 offset
    int32_t *p = (int32_t *)(code + 19);
    *p = rel_offset;
    memcpy(buf, code, sizeof(code));
    total += sizeof(code);
    // exit(0)
    uint8_t exit_code[] = {
        0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00,   // mov rax, 60
        0x48, 0x31, 0xff,                           // xor rdi, rdi
        0x0f, 0x05                                  // syscall
    };
    memcpy(buf + total, exit_code, sizeof(exit_code));
    total += sizeof(exit_code);
    // 写入字符串
    memcpy(buf + total, final, len);
    total += len;
    return total;
}