#include "forge/config.h"
#include "forge/graphics.h"
#include "forge/hook.h"
#include "forge/input.h"
#include "forge/log.h"
#include "forge/mem.h"
#include "forge/plugin.h"
#include "forge/proc.h"
#include "forge/singleton.h"
#include "forge/socket.h"
#include "forge/version.h"
#include <stdio.h>
#include <string.h>

void (*original_sApp_run)(void*) = NULL;

void sApp_run(void* app)
{
    Config config = forge_config_createDefault();
    if (R_FAILED(forge_config_load(&config))) {
        forge_log_trace("[forge] Failed to load config, using defaults");
    }

    forge_log_init(&config);
    forge_singleton_resolve();

    forge_log_info("[forge] Loading plugins...");
    forge_plugin_init();
    forge_plugin_loadPlugins();

    return original_sApp_run(app);
}

void forge_main()
{
    forge_log_trace("[forge] Initializing Forge...");
    forge_log_trace("[forge] Version %s", FORGE_VERSION);

    if (!FORGE_VERSION_IS_RELEASE) {
        forge_log_trace("[WARN] [forge] Running a non-release build, expect instability and crashes");
    }

    const int result = forge_socket_initDefault();
    if (result != 0) {
        forge_log_trace("[forge] Failed to initialize socket library: 0x%X", result);
    }

    const Result r = forge_hook_init();
    if (R_FAILED(r)) {
        forge_log_trace("[forge] Failed to initialize hooking API: 0x%08X", r);
        return;
    }

    if (!forge_graphics_init()) {
        forge_log_trace("[forge] Failed to initialize graphics");
    }

    forge_input_init();
    forge_singleton_init();

    forge_hook_create((void*)(g_mainTextAddr + 0xB8692C), (void*)(sApp_run), (void**)(&original_sApp_run));
}
