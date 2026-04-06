#pragma once

#include "switch.h"
#include <stdint.h>

void hookCreate(void* const target, void* const detour, void** original);
void* hookFunction(void* const target, void* const detour, Jit* const jit, const uintptr_t jit_size);
