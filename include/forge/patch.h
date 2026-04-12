#pragma once

#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Patch {
    u32 address;
    u32 size;
    void* patch_bytes;
    void* original_bytes;
    bool enabled;
} Patch;

Patch forge_patch_create(u32 address, const void* bytes, u32 length, bool enable);
void forge_patch_destroy(Patch* patch);

void forge_patch_enable(Patch* patch);
void forge_patch_disable(Patch* patch);

#ifdef __cplusplus
}
#endif
