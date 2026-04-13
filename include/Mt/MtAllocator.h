#pragma once
#ifdef __cplusplus

#include "Mt/MtObject.h"

enum class AllocatorType : u16 {
    Virtual = 0,
    Virtual64Kb,
    Physical,
    Physical64Kb,
    Physical16Mb,
    Develop,
    Cache,
    Shared,
    Onion,
    Garlic,
    Addr32Bit,
    Unknown,
};

class MtAllocator : public MtObject {
public:
    size_t used_size;
    size_t max_used_size;
    size_t capacity;
    char name_str[32];
    const char* name;
    AllocatorType type;
    u16 attr;
    void* owner;

    virtual void initialize(const char* name, AllocatorType type, size_t size, u16 attr) = 0;
    virtual size_t getMaxAvailableSize() const = 0;
    virtual void* alloc(size_t size, u32 alignment) = 0;
    virtual void* alloc(size_t size, u32 alignment, u32 tag) = 0;
    virtual void* alloc(size_t size, u32 alignment, u32 tag, u32 user_data) = 0;
    virtual void* allocTail(size_t size, u32 alignment) = 0;
    virtual void* allocTail(size_t size, u32 alignment, u32 tag) = 0;
    virtual void* allocTail(size_t size, u32 alignment, u32 tag, u32 user_data) = 0;
    virtual void free(void* ptr) = 0;
    virtual size_t getAllocSize(void* ptr) const = 0;
};

#endif
