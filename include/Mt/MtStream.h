#pragma once
#ifdef __cplusplus

#include "Mt/MtObject.h"

enum class SeekOrigin {
    Begin,
    Current,
    End
};

class MtStream : public MtObject {
public:
    virtual bool isReadable() const = 0;
    virtual bool isWritable() const = 0;
    virtual bool isSeekable() const = 0;
    virtual bool isAsyncReadable() const = 0;

    virtual u32 getPosition() const = 0;
    virtual void close() = 0;
    virtual void flush() = 0;

    virtual u32 read(void* buffer, u32 size) = 0;
    virtual u32 readAsync(void* buffer, u32 size) = 0;
    virtual void readAwait() = 0;
    virtual bool isAsyncReading() const = 0;

    virtual u32 write(const void* buffer, u32 size) = 0;

    virtual void setLength(u32 length) = 0;
    virtual u32 getLength() const = 0;

    virtual void seek(s32 offset, SeekOrigin origin) = 0;
    virtual void skip(u32 amount) = 0;

    virtual bool isAsyncOperationOk() const = 0;
};

#endif