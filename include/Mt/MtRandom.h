#pragma once
#ifdef __cplusplus

#include "MtFunc.h"
#include "forge/mem.h"

struct MtRandom {
    u32 x, y, z, w;

    static inline u32 nrand()
    {
        const auto instance = (MtRandom*)(g_mainTextAddr + 0x191EBB8);
        return nrand(*instance);
    }

    static inline u32 nrand(MtRandom& random)
    {
        return MtFunc::invoke<u32>(0x7CC110, &random);
    }
};

#endif
