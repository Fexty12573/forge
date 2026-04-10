#include "forge/patch.h"

#include <stdlib.h>
#include <string.h>

Patch forge_patch_create(u32 address, const void* bytes, u32 length, bool enable)
{
    Patch p = { 0 };

    if (length == 0) {
        // TODO: Log Error
        return p;
    }

    p.address = address;
    p.size = length;
    p.patch_bytes = malloc(length);
    p.original_bytes = malloc(length);

    memcpy(p.patch_bytes, bytes, length);
    memcpy(p.original_bytes, (const void*)address, length);

    if (enable) {
        forge_patch_enable(&p);
    }

    return p;
}

void forge_patch_destroy(Patch* patch)
{
    if (patch->address == 0) {
        return;
    }

    forge_patch_disable(patch);
}

void forge_patch_enable(Patch* patch)
{
    if (patch->address == 0 || patch->enabled) {
        return;
    }

    memcpy((void*)patch->address, patch->patch_bytes, patch->size);
    patch->enabled = true;
}

void forge_patch_disable(Patch* patch)
{
    if (patch->address == 0 || !patch->enabled) {
        return;
    }

    memcpy((void*)patch->address, patch->original_bytes, patch->size);
    patch->enabled = false;
}
