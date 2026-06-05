#include "forge/log.h"

void NX_NORETURN __assert_fail(const char* __assertion, const char* __file, unsigned __line, const char* __function);

void __assert_func(const char* file, int line, const char* function, const char* expr)
{
    forge_log_error("Assertion failed: %s, at %s:%d, in function %s", expr, file, line, function);
    __assert_fail(expr, file, line, function);
}
