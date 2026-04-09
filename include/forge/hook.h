#pragma once

#include "switch.h"
#include <stdint.h>

void forge_hook_create(void* const target, void* const detour, void** original);
