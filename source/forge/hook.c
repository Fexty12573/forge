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
#include <nn/os.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch/arm/cache.h>
#include <switch/kernel/svc.h>
#include <unistd.h>

// Need to use nnsdk facilities because `thread_local` is not wired up correctly.
static TlsSlot s_hookContextTlsSlot = -1;

Result forge_hook_init(void)
{
    if (s_hookContextTlsSlot != -1) {
        return KERNELRESULT(AlreadyExists);
    }

    Result r = nnosAllocateTlsSlot(&s_hookContextTlsSlot, NULL);
    if (R_FAILED(r) || s_hookContextTlsSlot == -1) {
        return r;
    }

    return 0;
}

__attribute__((noinline)) static void forge_hook_setContextInternal(void* context)
{
    nnosSetTlsValue(s_hookContextTlsSlot, context);
}

void* forge_hook_getContext(void) { return nnosGetTlsValue(s_hookContextTlsSlot); }

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

static void syncCodeForExecution(void* addr, size_t size)
{
    if (!addr || size == 0) {
        return;
    }

    armICacheInvalidate(addr, size);
    armDCacheFlush(addr, size);
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

static void* hookFunction(void* const target, void* const detour, Jit* const jit)
{
    if (!target || !detour) {
        forge_log_error("Invalid parameters: target=%p, detour=%p", target, detour);
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
        syncCodeForExecution(trampoline, (size_t)(total_bytes + 20));
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

    syncCodeForExecution((void*)(real_target), (size_t)(total_bytes));

    if (trampoline) {
        uintptr_t result_ptr = (uintptr_t)(jit->rx_addr);

        if (thumb_mode) {
            result_ptr |= 1;
        }

        return (void*)(result_ptr);
    }

    return NULL;
}

// ARM trampoline that stores an embedded context value into TLS then tail-jumps to the detour.
//   [0]  push {r0-r3, r12, lr}  ; 6 regs keeps stack 8-byte aligned
//   [1]  ldr  r0,  [pc, #12]    ; r0  = context   (word at offset 24)
//   [2]  ldr  r12, [pc, #12]    ; r12 = setContext (word at offset 28)
//   [3]  blx  r12
//   [4]  pop  {r0-r3, r12, lr}
//   [5]  ldr  pc,  [pc, #4]     ; tail-jump → detour (word at offset 32)
//   [6]  .word context
//   [7]  .word &forge_hook_setContextInternal
//   [8]  .word detour_addr
static void writeContextTrampoline(uint32_t* buf, void* context, void* detour)
{
    buf[0] = 0xE92D500F; // push {r0, r1, r2, r3, r12, lr}
    buf[1] = 0xE59F000C; // ldr r0,  [pc, #12]
    buf[2] = 0xE59FC00C; // ldr r12, [pc, #12]
    buf[3] = 0xE12FFF3C; // blx r12
    buf[4] = 0xE8BD500F; // pop {r0, r1, r2, r3, r12, lr}
    buf[5] = 0xE59FF004; // ldr pc, [pc, #4]
    buf[6] = (uint32_t)(uintptr_t)context;
    buf[7] = (uint32_t)(uintptr_t)forge_hook_setContextInternal;
    buf[8] = (uint32_t)(uintptr_t)detour;
}

Hook forge_hook_create(void* const target, void* const detour, void** original)
{
    Hook hook = { 0 };
    hook.target = target;
    hook.detour = detour;
    hook.has_ctx = false;

    if (original != NULL) {
        Result rc = jitCreate(&hook.jit, PAGE_SIZE);

        if (R_FAILED(rc)) {
            forge_log_error("Failed to create JIT trampoline: 0x%08X", rc);
            *original = NULL;
            return hook;
        }

        rc = jitTransitionToWritable(&hook.jit);

        if (R_FAILED(rc)) {
            forge_log_error("Failed to transition trampoline to writable: 0x%08X", rc);
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
                forge_log_error("Failed to transition trampoline to executable: 0x%08X", rc);
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

Hook forge_hook_createWithContext(void* const target, void* const detour, void** original, void* context)
{
    Hook hook = { 0 };
    hook.target = target;
    hook.detour = detour;
    hook.context = context;
    hook.has_ctx = true;

    Result rc = jitCreate(&hook.ctx_jit, PAGE_SIZE);
    if (R_FAILED(rc)) {
        forge_log_error("Failed to create JIT context trampoline: 0x%08X", rc);
        if (original)
            *original = NULL;
        return hook;
    }

    rc = jitTransitionToWritable(&hook.ctx_jit);
    if (R_FAILED(rc)) {
        forge_log_error("Failed to transition context trampoline to writable: 0x%08X", rc);
        jitClose(&hook.ctx_jit);
        if (original)
            *original = NULL;
        return hook;
    }

    writeContextTrampoline((uint32_t*)(hook.ctx_jit.rw_addr), context, detour);
    syncCodeForExecution((void*)(hook.ctx_jit.rw_addr), 36);

    rc = jitTransitionToExecutable(&hook.ctx_jit);
    if (R_FAILED(rc)) {
        forge_log_error("Failed to transition context trampoline to executable: 0x%08X", rc);
        jitClose(&hook.ctx_jit);
        if (original)
            *original = NULL;
        return hook;
    }

    void* ctx_trampoline = (void*)(hook.ctx_jit.rx_addr);

    if (original != NULL) {
        rc = jitCreate(&hook.jit, PAGE_SIZE);
        if (R_FAILED(rc)) {
            forge_log_error("Failed to create JIT trampoline: 0x%08X", rc);
            jitClose(&hook.ctx_jit);
            *original = NULL;
            return hook;
        }

        rc = jitTransitionToWritable(&hook.jit);
        if (R_FAILED(rc)) {
            forge_log_error("Failed to transition trampoline to writable: 0x%08X", rc);
            jitClose(&hook.jit);
            jitClose(&hook.ctx_jit);
            *original = NULL;
            return hook;
        }
    }

    void* final_trampoline = hookFunction(target, ctx_trampoline, &hook.jit);

    if (original != NULL) {
        if (final_trampoline != NULL) {
            rc = jitTransitionToExecutable(&hook.jit);
            if (R_FAILED(rc)) {
                forge_log_error("Failed to transition trampoline to executable: 0x%08X", rc);
                jitClose(&hook.jit);
                jitClose(&hook.ctx_jit);
                *original = NULL;
                return hook;
            }
            *original = final_trampoline;
            hook.original = final_trampoline;
        } else {
            jitClose(&hook.jit);
            jitClose(&hook.ctx_jit);
            *original = NULL;
        }
    }

    return hook;
}

Result forge_hook_updateContext(Hook* hook, void* new_ctx)
{
    if (!hook->has_ctx) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    Result rc = jitTransitionToWritable(&hook->ctx_jit);
    if (R_FAILED(rc)) {
        forge_log_error("Failed to transition JIT to writable: 0x%08X", rc);
        return rc;
    }

    uint32_t* buffer = hook->ctx_jit.rw_addr;
    buffer[6] = (uint32_t)(uintptr_t)new_ctx;

    syncCodeForExecution(buffer, 36);

    rc = jitTransitionToExecutable(&hook->ctx_jit);
    if (R_FAILED(rc)) {
        forge_log_error("Failed to transition JIT to executable: 0x%08X", rc);
        return rc;
    }

    return 0;
}
