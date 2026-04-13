#pragma once
#ifdef __cplusplus

#include "Mt/MtObject.h"
#include "Mt/MtStream.h"

class MtResource : public MtObject {
public:
    u16 magic_id;
    u16 magic_tag;
    char path[64];
    s32 ref_count;
    u32 attr;
    u32 state : 8;
    u32 quality : 3;
    u32 tag : 21;
    u32 size;
    u64 id;

    virtual u64 getUpdateTime(const char* full_path) const = 0;
    virtual const char* getExtension() const = 0;
    virtual bool compact(MtStream* stream) = 0;
    virtual bool create() = 0;
    virtual bool loadEnd() = 0;
    virtual bool load(MtStream* stream) = 0;
    virtual bool save(MtStream* stream) = 0;
    virtual bool convert(MtStream* stream) = 0;
    virtual bool convertEx(MtStream* stream, u32) = 0;
    virtual void clear() = 0;
};

#endif
