#pragma once

#include "forge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum ForgeOpenMode {
    ForgeOpenMode_Read = (1 << 0),
    ForgeOpenMode_Write = (1 << 1),
    ForgeOpenMode_Append = (1 << 2),
};

bool forge_fs_mountSaveData(void);
void forge_fs_unmountSaveData(void);
const char* forge_fs_getSaveDataMountPoint(void);
const char* forge_fs_getRomFsMountPoint(void);

bool forge_fs_fileExists(const char* path);
bool forge_fs_openFile(const char* path, enum ForgeOpenMode mode, u32* outHandle);
bool forge_fs_getFileSize(u32 handle, s64* outSize);
bool forge_fs_readFile(u32 handle, s64 offset, void* buffer, size_t size);
void forge_fs_closeFile(u32 handle);

#ifdef __cplusplus
}
#endif
