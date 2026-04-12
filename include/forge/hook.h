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
    Jit jit;
} Hook;

Hook forge_hook_create(void* const target, void* const detour, void** original);

#ifdef __cplusplus
}
#endif
