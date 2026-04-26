#pragma once

#include "switch.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hook {
    void* target;
    void* detour;
    void* original;
    void* context;
    Jit jit;
    Jit ctx_jit;
    bool has_ctx;
} Hook;

Result forge_hook_init(void);
Hook forge_hook_create(void* const target, void* const detour, void** original);
Hook forge_hook_createWithContext(void* const target, void* const detour, void** original, void* context);
void* forge_hook_getContext(void);

#ifdef __cplusplus
}
#endif
