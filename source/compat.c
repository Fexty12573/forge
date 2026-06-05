#include "forge/log.h"

#include <ctype.h>

void NX_NORETURN __assert_fail(const char* __assertion, const char* __file, unsigned __line, const char* __function);

void __assert_func(const char* file, int line, const char* function, const char* expr)
{
    forge_log_error("Assertion failed: %s, at %s:%d, in function %s", expr, file, line, function);
    __assert_fail(expr, file, line, function);
}

// clang-format off

// Fix for isspace/islower/isupper/tolower/toupper
const char _ctype_[1 + 256] = {
    0, // EOF
    /* 0x00-0x07 */ _C, _C, _C, _C, _C, _C, _C, _C,
    /* 0x08-0x0F */ _C, _S | _C, _S | _C, _S | _C, _S | _C, _S | _C, _C, _C,
    /* 0x10-0x17 */ _C, _C, _C, _C, _C, _C, _C, _C,
    /* 0x18-0x1F */ _C, _C, _C, _C, _C, _C, _C, _C,
    /* 0x20-0x27 */ _S | _B, _P, _P, _P, _P, _P, _P, _P,
    /* 0x28-0x2F */ _P, _P, _P, _P, _P, _P, _P, _P,
    /* 0x30-0x37 */ _N, _N, _N, _N, _N, _N, _N, _N,
    /* 0x38-0x3F */ _N, _N, _P, _P, _P, _P, _P, _P,
    /* 0x40-0x47 */ _P, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U,
    /* 0x48-0x4F */ _U, _U, _U, _U, _U, _U, _U, _U,
    /* 0x50-0x57 */ _U, _U, _U, _U, _U, _U, _U, _U,
    /* 0x58-0x5F */ _U, _U, _U, _P, _P, _P, _P, _P,
    /* 0x60-0x67 */ _P, _L | _X, _L | _X, _L | _X, _L | _X, _L | _X, _L | _X, _L,
    /* 0x68-0x6F */ _L, _L, _L, _L, _L, _L, _L, _L,
    /* 0x70-0x77 */ _L, _L, _L, _L, _L, _L, _L, _L,
    /* 0x78-0x7F */ _L, _L, _L, _P, _P, _P, _P, _C,
    // 0x80-0xFF default to 0
};

// clang-format on
