#pragma once

#include "switch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FORGE_CONFIG_PATH "app:/nativeNX/forge/config.ini"

typedef struct Config {
    const char* log_level;
    int menu_key;
} Config;

Config forge_config_createDefault(void);
Result forge_config_load(void);
void forge_config_destroy(void);
const Config* forge_config_get(void);

#ifdef __cplusplus
}
#endif
