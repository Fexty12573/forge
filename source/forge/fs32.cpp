#include "forge/fs.h"

#include <forge/nn/fs.h>

extern "C" bool forge_fs_fileExists(const char* path)
{
    nn::fs::FileHandle handle;
    if (nn::fs::OpenFile(&handle, path, nn::fs::OpenMode_Read).IsFailure()) {
        return false;
    }

    nn::fs::CloseFile(handle);
    return true;
}

extern "C" bool forge_fs_openFile(const char* path, enum ForgeOpenMode mode, u32* outHandle)
{
    nn::fs::FileHandle handle;
    if (nn::fs::OpenFile(&handle, path, mode).IsFailure()) {
        return false;
    }

    *outHandle = handle.data;
    return true;
}

extern "C" bool forge_fs_getFileSize(u32 handle, s64* outSize)
{
    return nn::fs::GetFileSize(outSize, { handle }).IsSuccess();
}

extern "C" bool forge_fs_readFile(u32 handle, s64 offset, void* buffer, size_t size)
{
    return nn::fs::ReadFile({ handle }, offset, buffer, size).IsSuccess();
}

extern "C" void forge_fs_closeFile(u32 handle)
{
    nn::fs::CloseFile({ handle });
}
