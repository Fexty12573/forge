#pragma once

#include <nn/types.h>

namespace nn::fs {

struct FileHandle {
    u32 data;
};

enum OpenMode {
    OpenMode_Read = 1 << 0,
    OpenMode_Write = 1 << 1,
    OpenMode_Append = 1 << 2,

    OpenMode_ReadWrite = OpenMode_Read | OpenMode_Write
};

Result OpenFile(FileHandle* outValue, const char* path, int mode) noexcept;
void CloseFile(FileHandle handle) noexcept;

Result GetFileSize(s64* outValue, FileHandle handle) noexcept;
Result ReadFile(FileHandle handle, s64 offset, void* buffer, size_t size) noexcept;

}
