#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <stdint.h>
#include <stddef.h>

size_t generateSyscall(uint8_t *buf, const char *msg, int newline);

#endif