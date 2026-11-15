#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool forge_graphics_init(void);
bool forge_graphics_isMenuVisible(void);

uint64_t forge_graphics_registerTexture(const void* texture, const void* view);
void forge_graphics_unregisterTexture(uint64_t textureId);

#ifdef __cplusplus
}
#endif
