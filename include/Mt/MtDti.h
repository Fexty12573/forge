#pragma once
#ifdef __cplusplus

#include "switch/types.h"

#include <string_view>

class MtObject;

class MtDti {
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

    u32 realSize() const { return size << 2; }

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
};

#endif
