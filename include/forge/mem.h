#pragma once

#include "switch.h"

extern u32 g_mainTextAddr;
extern u32 g_mainRodataAddr;
extern u32 g_mainDataAddr;
extern u32 g_mainBssAddr;
extern u32 g_mainHeapAddr;

void memSetup(void);

Result memGetMap(MemoryInfo* info, u32 addr);
u32 memGetMapAddr(u32 addr);
u32 memNextMap(u32);
u32 memNextMapOfType(u32, u32);
