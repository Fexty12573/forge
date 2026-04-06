#define MODULE_NAME "forge"
#define MODULE_NAME_LEN 5

__attribute__((section(".bss"))) char __nx_module_runtime[0xD0];

typedef struct {
    int unknown;
    int name_length;
    char name[MODULE_NAME_LEN + 1];
} ModuleName;

__attribute__((section(".rodata.module_name"))) const ModuleName module_name
    = { .unknown = 0, .name_length = MODULE_NAME_LEN, .name = MODULE_NAME };
