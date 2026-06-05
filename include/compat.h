#pragma once

// GCC on 32-bit ARM does not provide __uint128_t. Provide a 16-byte aligned
// stand-in so that nnheaders/nn/types.h compiles on this target.
#ifndef __SIZEOF_INT128__
#include <stdint.h>
typedef struct { uint64_t lo; uint64_t hi; } __attribute__((aligned(16))) __uint128_t;
typedef struct { int64_t lo; int64_t hi; } __attribute__((aligned(16))) __int128_t;
#endif
