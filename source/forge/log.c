#include "forge/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <switch.h>

static enum LogLevel s_logLevel = LOG_LEVEL_ERROR;

bool stricmp(const char* a, const char* b)
{
    while (*a && *b) {
        unsigned char ca = *a, cb = *b;
        if ((ca >= 'A' && ca <= 'Z' ? ca + 32 : ca) != (cb >= 'A' && cb <= 'Z' ? cb + 32 : cb)) {
            return false;
        }

        a++;
        b++;
    }

    return *a == *b;
}

void forge_log_init(const Config* config)
{
    if (!config->log_level) {
        return;
    }

    if (stricmp(config->log_level, "DEBUG")) {
        s_logLevel = LOG_LEVEL_DEBUG;
    } else if (stricmp(config->log_level, "INFO")) {
        s_logLevel = LOG_LEVEL_INFO;
    } else if (stricmp(config->log_level, "WARN")) {
        s_logLevel = LOG_LEVEL_WARN;
    } else if (stricmp(config->log_level, "ERROR")) {
        s_logLevel = LOG_LEVEL_ERROR;
    }
}

size_t forge_log_addPrefix(char* buffer, size_t buffer_size, enum LogLevel level)
{
    const char* prefix = "";
    switch (level) {
    case LOG_LEVEL_DEBUG:
        prefix = "[DEBUG] ";
        break;
    case LOG_LEVEL_INFO:
        prefix = "[INFO] ";
        break;
    case LOG_LEVEL_WARN:
        prefix = "[WARN] ";
        break;
    case LOG_LEVEL_ERROR:
        prefix = "[ERROR] ";
        break;
    }

    size_t prefix_len = strlen(prefix);
    if (prefix_len < buffer_size) {
        memcpy(buffer, prefix, prefix_len);
        return prefix_len;
    }

    return 0;
}

void forge_log(enum LogLevel level, const char* format, ...)
{
    if (level < s_logLevel) {
        return;
    }

    char buffer[520];
    const size_t offset = forge_log_addPrefix(buffer, sizeof(buffer), level);

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
    va_end(args);
    svcOutputDebugString(buffer, offset + (size_t)len);
}

enum LogLevel forge_log_getLevel(void)
{
    return s_logLevel;
}

void forge_log_trace(const char* format, ...)
{
    char buffer[520];
    const size_t offset = forge_log_addPrefix(buffer, sizeof(buffer), LOG_LEVEL_DEBUG);

    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
    va_end(args);

    svcOutputDebugString(buffer, offset + (size_t)len);
}
