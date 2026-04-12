#include "forge/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <switch.h>

void forge_log(const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    svcOutputDebugString(buffer, len);
}
