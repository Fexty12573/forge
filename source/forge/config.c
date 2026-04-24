#include "forge/config.h"
#include "forge/log.h"

#include "iniparser.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* strdup(const char* str)
{
    size_t len = strlen(str);
    char* copy = (char*)malloc(len + 1);
    if (copy) {
        memcpy(copy, str, len + 1);
    }

    return copy;
}

Result forge_config_load(Config* out_config)
{
    if (!out_config) {
        return KERNELRESULT(InvalidHandle);
    }

    if (access(FORGE_CONFIG_PATH, F_OK) != 0) {
        return KERNELRESULT(NotFound);
    }

    dictionary* dict = iniparser_load(FORGE_CONFIG_PATH);
    out_config->log_level = strdup(iniparser_getstring(dict, "log:level", "info"));

    iniparser_freedict(dict);

    return 0;
}

void forge_config_destroy(Config* config)
{
    if (!config) {
        return;
    }

    if (config->log_level) {
        free((char*)config->log_level);
        config->log_level = NULL;
    }
}
