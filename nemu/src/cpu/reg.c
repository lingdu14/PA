#include "nemu.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>


CPU_state cpu;

const char *regsl[] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
const char *regsw[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char *regsb[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

void reg_test() {
    srand(time(0));
    uint32_t sample[8];
    uint32_t eip_sample = rand();
    cpu.eip = eip_sample;

    int i;
    for (i = R_EAX; i <= R_EDI; i ++) {
        sample[i] = rand();
        reg_l(i) = sample[i];
        assert(reg_w(i) == (sample[i] & 0xffff));
    }

    assert(reg_b(R_AL) == (sample[R_EAX] & 0xff));
    assert(reg_b(R_AH) == ((sample[R_EAX] >> 8) & 0xff));
    assert(reg_b(R_BL) == (sample[R_EBX] & 0xff));
    assert(reg_b(R_BH) == ((sample[R_EBX] >> 8) & 0xff));
    assert(reg_b(R_CL) == (sample[R_ECX] & 0xff));
    assert(reg_b(R_CH) == ((sample[R_ECX] >> 8) & 0xff));
    assert(reg_b(R_DL) == (sample[R_EDX] & 0xff));
    assert(reg_b(R_DH) == ((sample[R_EDX] >> 8) & 0xff));

    assert(sample[R_EAX] == cpu.eax);
    assert(sample[R_ECX] == cpu.ecx);
    assert(sample[R_EDX] == cpu.edx);
    assert(sample[R_EBX] == cpu.ebx);
    assert(sample[R_ESP] == cpu.esp);
    assert(sample[R_EBP] == cpu.ebp);
    assert(sample[R_ESI] == cpu.esi);
    assert(sample[R_EDI] == cpu.edi);

    assert(eip_sample == cpu.eip);
}

void isa_reg_display() {
    // 打印优雅的表头
    printf("Name\tHex\t\tDecimal\n");
    printf("----------------------------------------\n");
    // 打印 32 位通用寄存器
    for (int i = 0; i < 8; i++) {
        printf("%s\t0x%08x\t%u\n", regsl[i], cpu.gpr[i]._32, cpu.gpr[i]._32);
    }
    // 打印 eip (程序计数器)
    printf("eip\t0x%08x\t%u\n", cpu.eip, cpu.eip);
    printf("----------------------------------------\n");
}

// 传入寄存器名称的字符串（比如 "eax"、"eip"），返回对应的数值
uint32_t isa_reg_str2val(const char *s, bool *success) {
    // 1. 匹配 32 位通用寄存器 (eax, ecx, etc.)
    for (int i = 0; i < 8; i++) {
        if (strcmp(regsl[i], s) == 0) {
            *success = true;
            return reg_l(i); // reg_l 宏已经在 reg.h 中定义
        }
    }

    // 2. 匹配 PC 寄存器 (eip)
    if (strcmp("eip", s) == 0 || strcmp("pc", s) == 0) {
        *success = true;
        return cpu.eip;
    }

    // 3. (可选) 匹配 16 位寄存器 (ax, cx, etc.)
    for (int i = 0; i < 8; i++) {
        if (strcmp(regsw[i], s) == 0) {
            *success = true;
            return reg_w(i); 
        }
    }

    // 4. (可选) 匹配 8 位寄存器 (al, cl, etc.)
    for (int i = 0; i < 8; i++) {
        if (strcmp(regsb[i], s) == 0) {
            *success = true;
            return reg_b(i);
        }
    }

    // 如果都没匹配上
    *success = false;
    printf("Unknown register: %s\n", s);
    return 0;
}
