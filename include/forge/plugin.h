#pragma once

#include "switch.h"

#ifdef __cplusplus
extern "C" {
#endif

void forge_plugin_init(void);
void forge_plugin_loadPlugins(void);

void forge_plugin_renderPluginInfo(void);

void forge_plugin_onImGuiInit(void* ctx, void* alloc, void* free);
void forge_plugin_onImGuiRender(void);
void forge_plugin_onImGuiFreeRender(void);

#ifdef __cplusplus
}
#endif
