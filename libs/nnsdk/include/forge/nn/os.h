#pragma once

#include "switch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int TlsSlot;

Result nnosAllocateTlsSlot(TlsSlot* out_slot, void (*destructor)(void*));
Result nnosFreeTlsSlot(TlsSlot slot);
Result nnosSetTlsValue(TlsSlot slot, void* value);
void* nnosGetTlsValue(TlsSlot slot);

#ifdef __cplusplus
}
#endif
