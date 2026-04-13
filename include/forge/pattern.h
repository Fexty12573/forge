#pragma once

#include "switch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PatternByte {
    u8 value;
    u8 mask;
} PatternByte;

typedef struct Pattern {
    PatternByte* pattern;
    size_t length;
} Pattern;

// Creates a proper pattern from a pattern string.
// Pattern string should have the format "1F BE E3 ?? 34"...
Pattern forge_pattern_create(const char* pattern);

// A bit pattern looks like so: "1100.1.. 000..1.1 00111010"
// with '.' representing a wildcard bit
Pattern forge_pattern_createBits(const char* bit_pattern);

void forge_pattern_destroy(Pattern pattern);

// Uses forge_pattern_create internally
u32 forge_pattern_find(const char* pattern);
u32 forge_pattern_findFrom(u32 start_addr, const char* pattern);

// Uses forge_pattern_createBits internally
u32 forge_pattern_findBits(const char* bits);
u32 forge_pattern_findBitsFrom(u32 start_addr, const char* bits);

u32 forge_pattern_findEx(Pattern pattern);
u32 forge_pattern_findFromEx(u32 start_addr, Pattern pattern);

#ifdef __cplusplus
}
#endif
