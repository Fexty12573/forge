#pragma once
#ifdef __cplusplus

#include "Mt/MtDti.h"
#include "Mt/MtStream.h"

#include <memory>

enum class OpenMode {
    Undefined = 0x10,
    Read,
    ReadAsync,
    Write,
    Append,
    ReadWrite,
    ReadWriteAppend
};

class MtFile : public MtObject {
public:
    virtual bool open(const char* path, OpenMode mode, bool interrupt = false) = 0;
    virtual void close() = 0;

    virtual u32 read(void* buffer, u32 size) = 0;
    virtual u32 readAsync(void* buffer, u32 size) = 0;
    virtual void readAwait() = 0;
    virtual bool isAsyncReading() const = 0;

    virtual u32 write(const void* buffer, u32 size) = 0;

    virtual void seek(s32 offset, SeekOrigin origin) = 0;
    virtual u32 getPosition() const = 0;
    virtual u32 getLength() const = 0;
    virtual void setLength(u32 length) = 0;

    virtual bool isReadable() const = 0;
    virtual bool isWritable() const = 0;
    virtual bool isAsyncReadable() const = 0;

    virtual u32 getAsyncTransferredSize() const = 0;

    static inline MtPtr<MtFile> open(const std::string& path, OpenMode mode)
    {
        auto file = MtDti::find("MtFile")->createInstance<MtFile>();
        return file->open(path.c_str(), mode) ? file : nullptr;
    }
};

#endif
