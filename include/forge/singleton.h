#pragma once

#include "forge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void forge_singleton_init(void);
void forge_singleton_resolve(void);
void* forge_singleton_getInstanceByName(const char* name);
void* forge_singleton_getInstanceById(u32 id);
u32 forge_singleton_getAllInstances(void** out, u32 max);

#ifdef __cplusplus
}
#endif
