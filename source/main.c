#include "forge/hook.h"
#include "forge/mem.h"
#include "forge/proc.h"
#include <stdio.h>
#include <string.h>

void (*originalAdjustSharpness)(void*, s32, bool) = NULL;

void adjustSharpness(void* unknown, s32 amount, bool ignoreSkills)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Sharpness adjusted by %ld", amount);
    svcOutputDebugString(buffer, strlen(buffer));
    originalAdjustSharpness(unknown, amount * 150, ignoreSkills);
}

void forge_main()
{
    forge_hook_create((void*)(g_mainTextAddr + 0x2AD288), (void*)(adjustSharpness), (void**)(&originalAdjustSharpness));
}
