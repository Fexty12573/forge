#define MODULE_NAME "mhguForge"
#define MODULE_NAME_LEN 9

__attribute__((section(".bss"))) char forge_module_runtime[0xD0];

typedef struct ModuleName {
    int attrib_type;
    int name_length;
    char name[MODULE_NAME_LEN + 1];
} ModuleName;

__attribute__((section(".rodata.module_name"))) const ModuleName forge_module_name
    = { .attrib_type = 0, .name_length = MODULE_NAME_LEN, .name = MODULE_NAME };
