#pragma once

#include "switch.h"

extern u32 g_mainTextAddr;
extern u32 g_mainRodataAddr;
extern u32 g_mainDataAddr;
extern u32 g_mainBssAddr;
extern u32 g_mainHeapAddr;

void forge_mem_init(void);
Result forge_mem_queryMemory(MemoryInfo* out_info, u32 addr);

u32 forge_mem_getMainTextAddr(void);
u32 forge_mem_getMainRodataAddr(void);
u32 forge_mem_getMainDataAddr(void);
u32 forge_mem_getMainBssAddr(void);
u32 forge_mem_getMainHeapAddr(void);
