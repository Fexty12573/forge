#pragma once
#ifdef __cplusplus

#include "forge/mem.h"
#include "switch/types.h"

namespace MtCRC {
inline __attribute__((naked)) u32 getCRC(const char* data, u32 crc)
{
    asm(
        "ldrb r2, [r0]\n"
        "cmp r2, #0\n"
        "beq 2f\n"
        "add r0, r0, #1\n"
        "1:\n"
        "uxtb r2, r2\n"
        "crc32b r1, r1, r2\n"
        "ldrb r2, [r0], #1\n"
        "cmp r2, #0\n"
        "bne 1b\n"
        "2:\n"
        "mov r0, r1\n"
        "bx lr\n"
    );
}

inline __attribute__((naked)) u32 getCRC(const char* data, u32 crc, u32 length)
{
    asm(
        "cmp r2, #0\n"
        "beq 2f\n"
        "1:\n"
        "ldrb r3, [r0], #1\n"
        "uxtb r3, r3\n"
        "crc32b r1, r1, r3\n"
        "subs r2, r2, #1\n"
        "bne 1b\n"
        "2:\n"
        "mov r0, r1\n"
        "bx lr\n"
    );
}
}

#endif
