/*
 *  MIT License
 *
 *  Copyright (c) 2025 Magic1-Mods
 *  Copyright (c) 2026 Jeffi
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include "forge/hook.h"
#include "forge/log.h"
#include "forge/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool isThumbMode(uintptr_t addr) { return (addr & 1) != 0; }

static uintptr_t getRealAddress(uintptr_t addr) { return addr & ~1; }

static int getInstructionSize(uintptr_t addr, bool thumb)
{
    if (thumb) {
        uint16_t ins = *(uint16_t*)(addr);

        if ((ins & 0xF800) == 0xF000 || (ins & 0xFF00) == 0xE800) {
            return 4;
        }

        return 2;
    }

    return 4;
}

static int calculateHookLength(uintptr_t addr, bool thumb, int min_length)
{
    int length = 0;
    int total_bytes = 0;

    while (total_bytes < min_length) {
        int ins_size = getInstructionSize(addr + total_bytes, thumb);
        total_bytes += ins_size;
        length++;
    }

    return length;
}

static void* hookFunction(void* const target, void* const detour, Jit* const jit)
{
    if (!target || !detour) {
        forge_log("Invalid parameters: target=%p, detour=%p", target, detour);
        return NULL;
    }

    uintptr_t target_addr = (uintptr_t)(target);
    uintptr_t hook_addr = (uintptr_t)(detour);
    bool thumb_mode = isThumbMode(target_addr);
    uintptr_t real_target = getRealAddress(target_addr);

    int min_bytes = thumb_mode ? 6 : 8;
    int instruction_count = calculateHookLength(real_target, thumb_mode, min_bytes);
    int total_bytes = 0;

    uintptr_t current_addr = real_target;
    for (int i = 0; i < instruction_count; i++) {
        total_bytes += getInstructionSize(current_addr, thumb_mode);
        current_addr += getInstructionSize(current_addr, thumb_mode);
    }

    uint32_t* trampoline = (uint32_t*)(jit->rw_addr);
    if (trampoline && jit->size >= total_bytes + 20u) {
        if (thumb_mode) {
            uint16_t* thumb_tramp = (uint16_t*)(trampoline);
            uint16_t* src = (uint16_t*)(real_target);
            uint16_t* dst = thumb_tramp;

            for (int i = 0; i < total_bytes / 2; i++) {
                dst[i] = src[i];
            }

            uintptr_t return_addr = real_target + total_bytes;
            if (return_addr % 2 == 0) {
                return_addr |= 1;
            }

            // ldr.w pc, [pc]
            thumb_tramp[total_bytes / 2] = 0xF8DF;
            thumb_tramp[total_bytes / 2 + 1] = 0xF000;
            *(uintptr_t*)(&thumb_tramp[total_bytes / 2 + 2]) = return_addr;
        } else {
            uint32_t* arm_tramp = trampoline;
            uint32_t* src = (uint32_t*)(real_target);
            uint32_t* dst = arm_tramp;

            for (int i = 0; i < instruction_count; i++) {
                dst[i] = src[i];
            }

            uintptr_t return_addr = real_target + total_bytes;
            arm_tramp[instruction_count] = 0xE51FF004; // ldr pc, [pc, #-4]
            arm_tramp[instruction_count + 1] = return_addr;
        }

        __builtin___clear_cache((char*)(trampoline), (char*)(trampoline) + total_bytes + 20);
    }

    if (thumb_mode) {
        uint16_t* code = (uint16_t*)(real_target);

        // Thumb hook: push lr; ldr r12, [pc, #4]; blx r12; pop {pc}; .addr hook_addr
        code[0] = 0xB500; // push {lr}
        code[1] = 0x9C02; // ldr r12, [pc, #8]
        code[2] = 0x47E0; // blx r12
        code[3] = 0xBD00; // pop {pc}
        code[4] = hook_addr & 0xFFFF;
        code[5] = hook_addr >> 16;
    } else {
        uint32_t* code = (uint32_t*)(real_target);

        code[0] = 0xE51FF004;
        code[1] = hook_addr;
    }

    __builtin___clear_cache((char*)(real_target), (char*)(real_target) + total_bytes);

    if (trampoline) {
        uintptr_t result_ptr = (uintptr_t)(jit->rx_addr);

        if (thumb_mode) {
            result_ptr |= 1;
        }

        return (void*)(result_ptr);
    }

    return NULL;
}

Hook forge_hook_create(void* const target, void* const detour, void** original)
{
    Hook hook = { 0 };
    hook.target = target;
    hook.detour = detour;

    if (original != NULL) {
        Result rc = jitCreate(&hook.jit, PAGE_SIZE);

        if (R_FAILED(rc)) {
            forge_log("Failed to create JIT trampoline: 0x%08X", rc);
            *original = NULL;
            return hook;
        }

        rc = jitTransitionToWritable(&hook.jit);

        if (R_FAILED(rc)) {
            forge_log("Failed to transition trampoline to writable: 0x%08X", rc);
            jitClose(&hook.jit);
            *original = NULL;
            return hook;
        }
    }

    void* final_trampoline = hookFunction(target, detour, &hook.jit);

    if (original != NULL) {
        if (final_trampoline != NULL) {
            Result rc = jitTransitionToExecutable(&hook.jit);

            if (R_FAILED(rc)) {
                forge_log("Failed to transition trampoline to executable: 0x%08X", rc);
                jitClose(&hook.jit);
                *original = NULL;
                return hook;
            }

            *original = final_trampoline;
            hook.original = final_trampoline;
        } else {
            jitClose(&hook.jit);
            *original = NULL;
        }
    }

    return hook;
}
