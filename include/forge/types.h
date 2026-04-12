#pragma once

#include "switch.h"

#define PAGE_SIZE 0x1000

#define R_ERRORONFAIL(r) \
    if (R_FAILED(r))     \
        *((Result*)0x69) = r;

typedef unsigned long ulong;
typedef unsigned int uint;
