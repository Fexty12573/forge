#include "forge/mem.h"
#include "forge/proc.h"
#include "switch.h"

typedef void (*FuncPtr)(void);

extern FuncPtr __preinit_array_start__[0], __preinit_array_end__[0];
extern FuncPtr __init_array_start__[0], __init_array_end__[0];
extern FuncPtr __fini_array_start__[0], __fini_array_end__[0];

void virtmemSetup(void);
void forge_main();

void forge_init(void)
{
    for (FuncPtr* func = __preinit_array_start__; func != __preinit_array_end__; func++)
        (*func)();

    for (FuncPtr* func = __init_array_start__; func != __init_array_end__; func++)
        (*func)();

    envSetup(NULL, forge_proc_getHandle(), NULL);
    virtmemSetup();
    forge_mem_init();
    forge_main();
}

void forge_fini(void)
{
    for (FuncPtr* func = __fini_array_start__; func != __fini_array_end__; func++)
        (*func)();
}
