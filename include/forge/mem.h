#pragma once

#include "switch.h"

extern u32 g_mainTextAddr;
extern u32 g_mainRodataAddr;
extern u32 g_mainDataAddr;
extern u32 g_mainBssAddr;
extern u32 g_mainHeapAddr;

void forge_mem_init(void);
