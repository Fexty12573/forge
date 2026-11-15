#pragma once
#include "forge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FORGE_NNOS_MUTEX_SIZE 0x14

void forge_nnosInitializeLightEvent(void* event, bool initially_signaled, int clear_mode);
void forge_nnosFinalizeLightEvent(void* event);
void forge_nnosSignalLightEvent(void* event);
void forge_nnosWaitLightEvent(void* event);
bool forge_nnosTryWaitLightEvent(void* event);
bool forge_nnosTimedWaitLightEvent(void* event, u64 timeout);
void forge_nnosClearLightEvent(void* event);

void forge_nnosInitializeMutex(void* mutex, bool recursive);
void forge_nnosLockMutex(void* mutex);
void forge_nnosUnlockMutex(void* mutex);
bool forge_nnosIsMutexLockedByCurrentThread(const void* mutex);

#ifdef __cplusplus
}
#endif
