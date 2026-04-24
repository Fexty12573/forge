#include "forge/config.h"
#include "forge/hook.h"
#include "forge/log.h"
#include "forge/mem.h"
#include "forge/plugin.h"
#include "forge/proc.h"
#include <stdio.h>
#include <string.h>

void (*original_sApp_run)(void*) = NULL;

void sApp_run(void* app)
{
    Config config;
    if (R_FAILED(forge_config_load(&config))) {
        forge_log_trace("[forge] Failed to load config, using defaults");
    }

    forge_log_init(&config);

    forge_log_info("[forge] Loading plugins...");
    forge_plugin_init();
    forge_plugin_loadPlugins();
    return original_sApp_run(app);
}

void forge_main()
{
    forge_hook_create((void*)(g_mainTextAddr + 0xB8692C), (void*)(sApp_run), (void**)(&original_sApp_run));
}
