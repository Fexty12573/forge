#include "forge/config.h"
#include "forge/log.h"

#include <iniparser.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Config s_config = { };

char* strdup(const char* str)
{
    size_t len = strlen(str);
    char* copy = (char*)malloc(len + 1);
    if (copy) {
        memcpy(copy, str, len + 1);
    }

    return copy;
}

int errorCallback(const char* msg, ...)
{
    char buffer[512];

    va_list args;
    va_start(args, msg);
    int len = vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);

    forge_log_trace("[forge] %s", buffer);

    return len;
}

Config forge_config_createDefault(void)
{
    Config cfg = {
        .log_level = "info",
        .menu_key = 66, // ForgeKey_F9
    };

    return cfg;
}

Result forge_config_load(void)
{
    s_config = forge_config_createDefault();

    if (access(FORGE_CONFIG_PATH, F_OK) != 0) {
        return KERNELRESULT(NotFound);
    }

    iniparser_set_error_callback(errorCallback);
    dictionary* dict = iniparser_load(FORGE_CONFIG_PATH);

    s_config.log_level = strdup(iniparser_getstring(dict, "log:level", "info"));
    s_config.menu_key = iniparser_getint(dict, "menu:key", 66);
    s_config.menu_font_size = (float)iniparser_getdouble(dict, "menu:font_size", 12.0);

    iniparser_freedict(dict);

    return 0;
}

void forge_config_destroy(void)
{
    if (s_config.log_level) {
        free((char*)s_config.log_level);
        s_config.log_level = NULL;
    }
}

const Config* forge_config_get(void)
{
    return &s_config;
}
