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
    forge_log("Loading plugins...");
    forge_plugin_loadPlugins();
    return original_sApp_run(app);
}

void forge_main()
{
    forge_hook_create((void*)(g_mainTextAddr + 0xB8692C), (void*)(sApp_run), (void**)(&original_sApp_run));
}
