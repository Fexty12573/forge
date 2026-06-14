#pragma once

#include "forge/types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool forge_fs_mountSaveData(void);
void forge_fs_unmountSaveData(void);
const char* forge_fs_getSaveDataMountPoint(void);
const char* forge_fs_getRomFsMountPoint(void);
bool forge_fs_fileExists(const char* path);

#ifdef __cplusplus
}
#endif
