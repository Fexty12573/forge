void forgeInit();

typedef void (*FuncPtr)(void);

extern FuncPtr __preinit_array_start__[0], __preinit_array_end__[0];
extern FuncPtr __init_array_start__[0], __init_array_end__[0];
extern FuncPtr __fini_array_start__[0], __fini_array_end__[0];

void __module_init(void)
{
    for (FuncPtr* func = __preinit_array_start__; func != __preinit_array_end__; func++)
        (*func)();

    for (FuncPtr* func = __init_array_start__; func != __init_array_end__; func++)
        (*func)();

    forgeInit();
}

void __module_fini(void)
{
    for (FuncPtr* func = __fini_array_start__; func != __fini_array_end__; func++)
        (*func)();
}
