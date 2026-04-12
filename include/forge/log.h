#pragma once

// Very simple logging utility for debugging purposes.

#ifdef __cplusplus
extern "C" {
#endif

void forge_log(const char* format, ...);

#ifdef __cplusplus
}
#endif
