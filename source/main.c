#include <stdio.h>
#include <string.h>

#include "forge/hook.h"
#include "forge/mem.h"
#include "forge/proc.h"
#include "switch.h"

typedef void (*AdjustSharpness)(void*, s32, bool);
AdjustSharpness originalAdjustSharpness = NULL;

void adjustSharpness(void* unknown, s32 amount, bool ignoreSkills)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Sharpness adjusted by %ld", amount);
    svcOutputDebugString(buffer, strlen(buffer));
    originalAdjustSharpness(unknown, amount * 150, ignoreSkills);
}

void forgeMain()
{
    hookCreate((void*)(g_mainTextAddr + 0x2AD288), (void*)(adjustSharpness), (void**)(&originalAdjustSharpness));
}

void virtmemSetup(void);

void forgeInit()
{
    envSetup(NULL, procGetHandle(), NULL);
    virtmemSetup();
    memSetup();
    forgeMain();
}
