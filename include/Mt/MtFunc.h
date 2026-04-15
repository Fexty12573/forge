#pragma once
#ifdef __cplusplus

#include "forge/mem.h"
#include <concepts>
#include <utility>

namespace MtFunc {
template <typename F>
    requires std::is_function_v<F>
inline F* get(u32 offset)
{
    return (F*)(g_mainTextAddr + offset);
}

template <typename Ret, typename... Args>
inline Ret invoke(u32 offset, Args... args)
{
    return get<Ret(Args)>(offset)(args...);
}
}

#endif
