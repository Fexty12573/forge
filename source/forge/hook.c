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
#include "forge/mem.h"
#include "forge/types.h"
#include "forge/wrapper.h"
#include <forge/nn/os.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch/arm/cache.h>
#include <switch/kernel/svc.h>
#include <unistd.h>

#define HOOK_JIT_SIZE (4 * PAGE_SIZE)
#define HOOK_JIT_MAX_CHUNKS 2
#define HOOK_CONTEXT_STUB_SIZE 44

typedef struct JitChunk {
    Jit jit;
    size_t used;
} JitChunk;

typedef struct JitSlot {
    void* rw;
    void* rx;
} JitSlot;

// Need to use nnsdk facilities because `thread_local` is not wired up correctly.
static TlsSlot s_hookContextTlsSlot = -1;

// Slots are never reused, so writing a new slot can't affect threads executing an existing one.
static JitChunk s_jitChunks[HOOK_JIT_MAX_CHUNKS];
static int s_jitChunkCount = 0;
static u8 s_jitMutex[FORGE_NNOS_MUTEX_SIZE] __attribute__((aligned(4)));

Result forge_hook_init(void)
{
    if (s_hookContextTlsSlot != -1) {
        return KERNELRESULT(AlreadyExists);
    }

    Result r = nnosAllocateTlsSlot(&s_hookContextTlsSlot, NULL);
    if (R_FAILED(r) || s_hookContextTlsSlot == -1) {
        return r;
    }

    forge_nnosInitializeMutex(s_jitMutex, false);

    return 0;
}

__attribute__((noinline)) static void forge_hook_setContextInternal(void* context)
{
    nnosSetTlsValue(s_hookContextTlsSlot, context);
}

void* forge_hook_getContext(void) { return nnosGetTlsValue(s_hookContextTlsSlot); }

static bool createJitChunk(JitChunk* chunk)
{
    Result rc = jitCreate(&chunk->jit, HOOK_JIT_SIZE);
    if (R_FAILED(rc)) {
        forge_log_error("Failed to create hook JIT: 0x%08X", rc);
        return false;
    }

    // Writing while other slots execute requires the RW and RX mappings to coexist.
    if (chunk->jit.type != JitType_CodeMemory) {
        forge_log_error("Unsupported hook JIT type: %d", chunk->jit.type);
        jitClose(&chunk->jit);
        return false;
    }

    chunk->used = 0;
    return true;
}

static bool allocJitSlotLocked(size_t size, JitSlot* out)
{
    JitChunk* chunk = s_jitChunkCount > 0 ? &s_jitChunks[s_jitChunkCount - 1] : NULL;

    if (chunk == NULL || chunk->used + size > chunk->jit.size) {
        if (s_jitChunkCount == HOOK_JIT_MAX_CHUNKS) {
            forge_log_error("Hook JIT is full");
            return false;
        }

        chunk = &s_jitChunks[s_jitChunkCount];
        if (!createJitChunk(chunk)) {
            return false;
        }

        s_jitChunkCount++;
    }

    out->rw = (u8*)chunk->jit.rw_addr + chunk->used;
    out->rx = (u8*)chunk->jit.rx_addr + chunk->used;
    chunk->used += size;
    return true;
}

static bool allocJitSlot(size_t size, JitSlot* out)
{
    size = (size + 7) & ~(size_t)7;

    forge_nnosLockMutex(s_jitMutex);
    bool ok = allocJitSlotLocked(size, out);
    forge_nnosUnlockMutex(s_jitMutex);

    return ok;
}

static bool isThumbMode(uintptr_t addr) { return (addr & 1) != 0; }

static uintptr_t getRealAddress(uintptr_t addr) { return addr & ~1; }

static int getInstructionSize(uintptr_t addr, bool thumb)
{
    if (thumb) {
        uint16_t ins = *(uint16_t*)(addr);

        if ((ins & 0xF800) >= 0xE800) {
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

static void syncCodeForExecution(void* rw_addr, void* rx_addr, size_t size)
{
    armDCacheFlush(rw_addr, size);
    armICacheInvalidate(rx_addr, size);
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

static bool hookFunction(void* const target, void* const detour, void** original)
{
    if (!target || !detour) {
        forge_log_error("Invalid parameters: target=%p, detour=%p", target, detour);
        return false;
    }

    uintptr_t target_addr = (uintptr_t)(target);
    uintptr_t hook_addr = (uintptr_t)(detour);
    bool thumb_mode = isThumbMode(target_addr);
    uintptr_t real_target = getRealAddress(target_addr);

    int min_bytes = thumb_mode && (real_target & 2) ? 10 : 8;
    int instruction_count = calculateHookLength(real_target, thumb_mode, min_bytes);
    int total_bytes = 0;

    uintptr_t current_addr = real_target;
    for (int i = 0; i < instruction_count; i++) {
        total_bytes += getInstructionSize(current_addr, thumb_mode);
        current_addr += getInstructionSize(current_addr, thumb_mode);
    }

    if (original != NULL) {
        const size_t jump_padding = thumb_mode && (total_bytes & 2) ? 2 : 0;
        size_t trampoline_size = (size_t)(total_bytes + jump_padding + 8);
        JitSlot slot;

        if (!allocJitSlot(trampoline_size, &slot)) {
            return false;
        }

        if (thumb_mode) {
            uint16_t* thumb_tramp = (uint16_t*)(slot.rw);
            uint16_t* src = (uint16_t*)(real_target);

            for (int i = 0; i < total_bytes / 2; i++) {
                thumb_tramp[i] = src[i];
            }

            uintptr_t return_addr = real_target + total_bytes;
            if (return_addr % 2 == 0) {
                return_addr |= 1;
            }

            if (jump_padding) {
                thumb_tramp[total_bytes / 2] = 0xBF00; // nop; align literal load
            }
            const size_t jump = (total_bytes + jump_padding) / 2;
            // ldr.w pc, [pc]
            thumb_tramp[jump] = 0xF8DF;
            thumb_tramp[jump + 1] = 0xF000;
            memcpy(&thumb_tramp[jump + 2], &return_addr, sizeof(return_addr));
        } else {
            uint32_t* arm_tramp = (uint32_t*)(slot.rw);
            uint32_t* src = (uint32_t*)(real_target);

            for (int i = 0; i < instruction_count; i++) {
                arm_tramp[i] = src[i];
            }

            uintptr_t return_addr = real_target + total_bytes;
            arm_tramp[instruction_count] = 0xE51FF004; // ldr pc, [pc, #-4]
            arm_tramp[instruction_count + 1] = return_addr;
        }

        syncCodeForExecution(slot.rw, slot.rx, trampoline_size);

        uintptr_t result_ptr = (uintptr_t)(slot.rx);
        if (thumb_mode) {
            result_ptr |= 1;
        }

        *original = (void*)(result_ptr);
    }

    // Thumb literal loads use Align(PC, 4), so allow a leading NOP at +2.
    const size_t prefix = thumb_mode && (real_target & 2) ? 2 : 0;
    u8 branch[10];
    if (thumb_mode) {
        const uint16_t nop = 0xBF00;
        const uint16_t load_pc[] = { 0xF8DF, 0xF000 };
        memcpy(branch, &nop, prefix);
        memcpy(branch + prefix, load_pc, sizeof(load_pc));
        memcpy(branch + prefix + 4, &hook_addr, sizeof(hook_addr));
    } else {
        const uint32_t load_pc = 0xE51FF004;
        memcpy(branch, &load_pc, sizeof(load_pc));
        memcpy(branch + 4, &hook_addr, sizeof(hook_addr));
    }
    const Result rc = forge_mem_write(real_target, branch, prefix + 8);
    if (R_FAILED(rc)) {
        forge_log_trace("Failed to write hook at %p: 0x%08X", target, rc);
        return false;
    }

    syncCodeForExecution((void*)(real_target), (void*)(real_target), (size_t)(total_bytes));

    return true;
}

// ARM stub that stores the context into TLS then tail-jumps to the detour.
// The context is read through the RW mapping so it can be updated without touching code.
//   [0]  push {r0-r3, r12, lr}  ; 6 regs keeps stack 8-byte aligned
//   [1]  ldr  r0,  [pc, #16]    ; r0  = &context  (word at offset 28)
//   [2]  ldr  r0,  [r0]
//   [3]  ldr  r12, [pc, #12]    ; r12 = setContext (word at offset 32)
//   [4]  blx  r12
//   [5]  pop  {r0-r3, r12, lr}
//   [6]  ldr  pc,  [pc, #4]     ; tail-jump → detour (word at offset 36)
//   [7]  .word &context (RW address)
//   [8]  .word &forge_hook_setContextInternal
//   [9]  .word detour_addr
//   [10] .word context
static void** writeContextStub(const JitSlot* slot, void* context, void* detour)
{
    uint32_t* buf = (uint32_t*)(slot->rw);

    buf[0] = 0xE92D500F; // push {r0, r1, r2, r3, r12, lr}
    buf[1] = 0xE59F0010; // ldr r0,  [pc, #16]
    buf[2] = 0xE5900000; // ldr r0,  [r0]
    buf[3] = 0xE59FC00C; // ldr r12, [pc, #12]
    buf[4] = 0xE12FFF3C; // blx r12
    buf[5] = 0xE8BD500F; // pop {r0, r1, r2, r3, r12, lr}
    buf[6] = 0xE59FF004; // ldr pc, [pc, #4]
    buf[7] = (uint32_t)(uintptr_t)&buf[10];
    buf[8] = (uint32_t)(uintptr_t)forge_hook_setContextInternal;
    buf[9] = (uint32_t)(uintptr_t)detour;
    buf[10] = (uint32_t)(uintptr_t)context;

    return (void**)&buf[10];
}

Hook forge_hook_create(void* const target, void* const detour, void** original)
{
    Hook hook = { 0 };
    hook.target = target;
    hook.detour = detour;
    hook.has_ctx = false;

    if (original != NULL) {
        *original = NULL;
    }

    if (hookFunction(target, detour, original) && original != NULL) {
        hook.original = *original;
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

    if (original != NULL) {
        *original = NULL;
    }

    JitSlot slot;
    if (!allocJitSlot(HOOK_CONTEXT_STUB_SIZE, &slot)) {
        forge_log_error("Failed to allocate context stub");
        return hook;
    }

    hook.ctx_cell = writeContextStub(&slot, context, detour);
    syncCodeForExecution(slot.rw, slot.rx, HOOK_CONTEXT_STUB_SIZE);

    if (hookFunction(target, slot.rx, original) && original != NULL) {
        hook.original = *original;
    }

    return hook;
}

Result forge_hook_updateContext(Hook* hook, void* new_ctx)
{
    if (!hook->has_ctx || hook->ctx_cell == NULL) {
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    __atomic_store_n(hook->ctx_cell, new_ctx, __ATOMIC_RELEASE);
    hook->context = new_ctx;

    return 0;
}
