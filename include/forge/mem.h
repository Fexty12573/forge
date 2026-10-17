#pragma once

#include "switch.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u32 g_mainTextAddr;
extern u32 g_mainRodataAddr;
extern u32 g_mainDataAddr;
extern u32 g_mainBssAddr;
extern u32 g_mainHeapAddr;

void forge_mem_init(void);
Result forge_mem_queryMemory(MemoryInfo* out_info, u32 addr);
Result forge_mem_write(u32 addr, const void* data, size_t size);

u32 forge_mem_getMainTextAddr(void);
u32 forge_mem_getMainRodataAddr(void);
u32 forge_mem_getMainDataAddr(void);
u32 forge_mem_getMainBssAddr(void);
u32 forge_mem_getMainHeapAddr(void);

#ifdef __cplusplus
}
#endif
