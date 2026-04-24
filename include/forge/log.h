#pragma once

#include "forge/config.h"

#ifdef __cplusplus
extern "C" {
#endif

enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3
};

void forge_log_init(const Config* config);
void forge_log(enum LogLevel level, const char* format, ...);
enum LogLevel forge_log_getLevel(void);
void forge_log_trace(const char* format, ...);

#define forge_log_debug(format, ...) forge_log(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define forge_log_info(format, ...) forge_log(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define forge_log_warn(format, ...) forge_log(LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#define forge_log_error(format, ...) forge_log(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
