#pragma once
#ifdef __cplusplus

#include <memory>

class MtObject;

template <std::derived_from<MtObject> T>
using MtPtr = std::shared_ptr<T>;

template <std::derived_from<MtObject> T>
using MtWeakPtr = std::weak_ptr<T>;

#endif
