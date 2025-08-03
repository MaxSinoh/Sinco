#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "elf.h"
#include "syscall.h"

#define MAX_VARS 128

typedef enum { VAR_STRING, VAR_NUMBER } VarType;

typedef struct {
    char name[32];
    VarType type;
    char str_val[128];
    int num_val;
} Variable;

Variable vars[MAX_VARS];
int var_count = 0;

// 查找变量
Variable *find_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) return &vars[i];
    }
    return NULL;
}

// 设置变量值
void set_var(const char *name, const char *value) {
    Variable *v = find_var(name);
    if (!v) {
        if (var_count >= MAX_VARS) {
            fprintf(stderr, "Error: Number of variables exceeds the upper limit.\n");
            exit(1);
        }
        v = &vars[var_count++];
        strncpy(v->name, name, sizeof(v->name) - 1);
    }

    // 判断是否为数字
    int is_num = 1;
    for (size_t i = 0; i < strlen(value); i++) {
        if (!isdigit((unsigned char)value[i])) { is_num = 0; break; }
    }

    if (is_num) {
        v->type = VAR_NUMBER;
        v->num_val = atoi(value);
    } else {
        v->type = VAR_STRING;
        strncpy(v->str_val, value, sizeof(v->str_val) - 1);
    }
}

// 生成系统调用
size_t generate_output(uint8_t *code, const char *target, int newline) {
    Variable *v = find_var(target);
    char buffer[128];

    if (v) {
        if (v->type == VAR_NUMBER) {
            snprintf(buffer, sizeof(buffer), "%d", v->num_val);
            return generateSyscall(code, buffer, newline);
        } else {
            return generateSyscall(code, v->str_val, newline);
        }
    } else {
        // 如果没有找到变量，直接输出目标字符串或数字
        return generateSyscall(code, target, newline);
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1 && strcmp(argv[1], "-v") == 0) {
        fprintf(stderr, "Sinco Language Compiler v0.0.6\n");
        return 1;
    }
    if (argc < 4 || strcmp(argv[2], "-o") != 0) {
        fprintf(stderr, "Usage: %s source.sc -o output\n", argv[0]);
        return 1;
    }
    // 打开源代码文件
    FILE *src = fopen(argv[1], "r");
    if (!src) {
        perror("Cannot open the source file.");
        return 1;
    }
    // 读取源代码并生成ELF文件
    uint8_t code[MAX_CODE];
    size_t code_len = 0;
    char line[256];

    while (fgets(line, sizeof(line), src)) {
        char str[128], var_name[32], value[128];

        // 移除行尾的换行符和回车符
        line[strcspn(line, "\r\n")] = 0;

        // 输出字符串或变量值
        if (sscanf(line, "out('%127[^']');", str) == 1 || sscanf(line, "out(\"%127[^\"]\");", str) == 1) {
            code_len += generate_output(code + code_len, str, 0);
        }
        else if (sscanf(line, "outln('%127[^']');", str) == 1 || sscanf(line, "outln(\"%127[^\"]\");", str) == 1) {
            code_len += generate_output(code + code_len, str, 1);
        }
        // 输出变量值
        else if (sscanf(line, "out(%31[^)]);", var_name) == 1) {
            code_len += generate_output(code + code_len, var_name, 0);
        }
        else if (sscanf(line, "outln(%31[^)]);", var_name) == 1) {
            code_len += generate_output(code + code_len, var_name, 1);
        }
        // 设置变量值
        else if (sscanf(line, "%31[^=]=\"%127[^\"]\";", var_name, value) == 2 || 
                 sscanf(line, "%31[^=]='%127[^']';", var_name, value) == 2) {
            // 移除变量名前的空格
            char *trim = var_name;
            while (isspace((unsigned char)*trim)) trim++;
            set_var(trim, value);
        }
        // 设置变量值（数字）
        else if (sscanf(line, "%31[^=]=%127[^;];", var_name, value) == 2) {
            char *trim = var_name;
            while (isspace((unsigned char)*trim)) trim++;
            set_var(trim, value);
        }
        else {
            fprintf(stderr, "Unknown command: %s\n", line);
        }
    }
    // 关闭源代码文件并写入ELF文件
    fclose(src);
    writeELF(argv[3], code, code_len);
    return 0;
}