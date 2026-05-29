#pragma once
#ifdef __cplusplus

#include "Mt/MtCRC.h"
#include "Mt/MtFunc.h"
#include "Mt/MtObject.h"
#include "Mt/MtPtr.h"
#include "switch/types.h"

#include <string_view>

template <std::derived_from<MtObject> T>
struct MtDeleter {
    void operator()(T* ptr) const
    {
        // `delete` dispatches virtually to the deleting destructor (D0, vtable
        // slot +0x04), which both destructs the object and frees it with the
        // game's allocator -- i.e. exactly what the old `destroy()` slot did.
        delete static_cast<MtObject*>(ptr);
    }
};

template <std::derived_from<MtObject> T>
MtPtr<T> toMtPtr(MtObject* ptr)
{
    return MtPtr<T> { ptr, MtDeleter<T> { } };
}

class __packed MtDti {
public:
    const char* name;
    class MtDti* next;
    class MtDti* child;
    class MtDti* parent;
    class MtDti* link;
    u32 size : 23;
    u32 allocator_index : 6;
    u32 attr : 3;
    u32 id;

    virtual ~MtDti() = 0;
    virtual MtObject* newInstance() const = 0;
    virtual MtObject* instantiate(MtObject* obj) const = 0;
    virtual MtObject* instantiateArray(MtObject* objects, s64 count) const = 0;

    template <std::derived_from<MtObject> T>
    MtPtr<T> createInstance() const
    {
        return toMtPtr<T>(newInstance());
    }

    u32 realSize() const
    {
        return size << 2;
    }

    bool inheritsFrom(const MtDti* other) const
    {
        if (this == other) {
            return true;
        }

        for (auto dti = parent; dti != nullptr; dti = dti->parent) {
            if (other == dti) {
                return true;
            }
        }

        return false;
    }

    bool inheritsFrom(u32 id) const
    {
        if (this->id == id) {
            return true;
        }

        for (auto dti = parent; dti != nullptr; dti = dti->parent) {
            if (dti->id == id) {
                return true;
            }
        }

        return false;
    }

    bool inheritsFrom(std::string_view name) const
    {
        if (this->name == name) {
            return true;
        }

        for (auto dti = parent; dti != nullptr; dti = dti->parent) {
            if (dti->name == name) {
                return true;
            }
        }

        return false;
    }

    static inline u32 makeId(std::string_view name)
    {
        return MtCRC::getCRC(name.data(), 0xFFFFFFFF, name.size()) & 0x7FFFFFFF;
    }

    static inline MtDti* find(u32 id)
    {
        return MtFunc::invoke<MtDti*>(0x1177F68, id);
    }

    static inline MtDti* find(std::string_view name)
    {
        return find(makeId(name));
    }
};

static_assert(sizeof(MtDti) == 0x20, "");

#endif
