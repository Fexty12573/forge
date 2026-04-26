#pragma once

#include "switch.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FORGE_CONFIG_PATH "app:/nativeNX/forge/config.ini"

typedef struct Config {
    const char* log_level;
} Config;

Config forge_config_createDefault(void);
Result forge_config_load(Config* out_config);
void forge_config_destroy(Config* config);

#ifdef __cplusplus
}
#endif
