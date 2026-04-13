#pragma once
#ifdef __cplusplus

#include "Mt/MtObject.h"

#include <concepts>

template <std::derived_from<MtObject> T>
class MtArray : public MtObject {
public:
    using value_type = T*;
    using pointer = T**;
    using reference = T*&;
    using const_reference = T* const&;
    using iterator = T**;
    using const_iterator = T* const*;
    using size_type = size_t;

    u32 length;
    u32 capacity;
    bool auto_delete;
    pointer items;

    reference operator[](size_t index) { return items[index]; }
    const_reference operator[](size_t index) const { return items[index]; }
    reference at(size_t index) { return items[index]; }
    const_reference at(size_t index) const { return items[index]; }

    size_type size() const { return length; }
    iterator begin() { return items; }
    iterator end() { return items + length; }
    const_iterator begin() const { return items; }
    const_iterator end() const { return items + length; }
};

#endif
