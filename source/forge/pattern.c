#include "forge/pattern.h"
#include "forge/mem.h"

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


u8* search(u8* begin, u8* end, Pattern pattern)
{
    const PatternByte* p_end = pattern.pattern + pattern.length;

    while (true) {
        u8* it = begin;
        for (const PatternByte* p_it = pattern.pattern;; it++, p_it++) {
            if (p_it == p_end) {
                return begin;
            }

            if (it == end) {
                return end;
            }

            if ((*it & p_it->mask) != p_it->value) {
                break;
            }
        }

        begin++;
    }
}

Pattern forge_pattern_create(const char* pattern)
{
    const size_t len = strlen(pattern);

    // Approximation of the number of bytes in the pattern,
    // since each byte is represented by 2 chars and a space
    size_t capacity = len / 3 + 1;
    PatternByte* pat = malloc(sizeof(PatternByte) * capacity);

    size_t pat_index = 0;
    for (size_t i = 0; i < len; i++) {
        if (pattern[i] == ' ')
            continue;
        if (pattern[i] == '?') {
            pat[pat_index++] = (PatternByte){ 0, 0 };

            // Second '?' is optional, so skip it if it's there
            if (i + 1 < len && pattern[i + 1] == '?') {
                i++;
            }
        } else {
            pat[pat_index++] = (PatternByte){
                .value = (u8)strtol(&pattern[i], NULL, 16),
                .mask = 0xFF
            };

            i += 1; // Skip the next char since it's part of the current byte
        }

        if (pat_index >= capacity) {
            capacity = (capacity * 3) / 2; // Increase capacity by 1.5x
            pat = realloc(pat, sizeof(PatternByte) * capacity);
        }
    }

    Pattern result = { .pattern = pat, .length = pat_index };
    return result;
}

Pattern forge_pattern_createBits(const char* bit_pattern)
{
    const size_t len = strlen(bit_pattern);

    // Approximation of the number of bytes in the pattern,
    // since each byte is represented by 8 chars and a space
    size_t capacity = len / 8 + 1;
    PatternByte* pat = malloc(sizeof(PatternByte) * capacity);

    size_t pat_index = 0;
    u8 cur_byte = 0;
    u8 cur_mask = 0;
    int bit = 0;
    for (size_t i = 0; i < len; i++) {
        const char c = bit_pattern[i];
        bit += 1;
        cur_byte = (cur_byte << 1) + (c == '1');
        cur_mask = (cur_byte << 1) + (c != '.');

        if (bit == 8) {
            bit = 0;
            cur_byte = 0;
            cur_mask = 0;
            pat[pat_index++] = (PatternByte){
                .value = cur_byte,
                .mask = cur_mask
            };

            if (pat_index >= capacity) {
                capacity = (capacity * 3) / 2; // Increase capacity by 1.5x
                pat = realloc(pat, sizeof(PatternByte) * capacity);
            }
        }
    }

    Pattern result = { .pattern = pat, .length = pat_index };
    return result;
}

void forge_pattern_destroy(Pattern pattern)
{
    if (pattern.pattern) {
        free(pattern.pattern);
    }
}

u32 forge_pattern_find(const char* pattern)
{
    return forge_pattern_findFrom(g_mainTextAddr, pattern);
}

u32 forge_pattern_findFrom(u32 start_addr, const char* pattern)
{
    const Pattern pat = forge_pattern_create(pattern);
    const u32 result = forge_pattern_findFromEx(start_addr, pat);
    forge_pattern_destroy(pat);

    return result;
}

u32 forge_pattern_findBits(const char* bits)
{
    return forge_pattern_findBitsFrom(g_mainTextAddr, bits);
}

u32 forge_pattern_findBitsFrom(u32 start_addr, const char* bits)
{
    const Pattern pat = forge_pattern_createBits(bits);
    const u32 result = forge_pattern_findFromEx(start_addr, pat);
    forge_pattern_destroy(pat);

    return result;
}

u32 forge_pattern_findEx(Pattern pattern)
{
    return forge_pattern_findFromEx(g_mainTextAddr, pattern);
}

u32 forge_pattern_findFromEx(u32 start_addr, Pattern pattern)
{
    MemoryInfo mi;
    if (R_FAILED(forge_mem_queryMemory(&mi, start_addr))) {
        return 0;
    }

    const u32 end_addr = start_addr + mi.size;

    u8* begin = (u8*)start_addr;
    u8* end = (u8*)end_addr;

    u8* result = search(begin, end, pattern);
    if (result != end) {
        return (u32)result;
    }

    return 0;
}
