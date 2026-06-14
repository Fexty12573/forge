#include "forge/fs.h"
#include "forge/mem.h"
#include "forge/singleton.h"

#include <nn/fs.h>
#include <nn/os.h>

constexpr size_t kMountSaveDataOffset = 0x878DA0;
constexpr size_t kJobSafeOffset = 0x212041C;
constexpr auto kSaveDataMountPoint = "savedata";
constexpr auto kRomFsMountPoint = "app";

namespace {
bool isJobSafe()
{
    return *(bool*)(g_mainTextAddr + kJobSafeOffset);
}

bool isThreadSafe(void* saveData)
{
    return *(bool*)((char*)saveData + 0x18);
}

nn::os::MutexType* getMutex(void* saveData)
{
    return (nn::os::MutexType*)((char*)saveData + 0x4);
}

int& mountCount(void* saveData)
{
    return *(int*)((char*)saveData + 0xF1C);
}
}

extern "C" bool forge_fs_mountSaveData(void)
{
    const auto mount = (int (*)(void*))(g_mainTextAddr + kMountSaveDataOffset);
    const auto saveData = forge_singleton_getInstanceByName("sSavedata");
    if (saveData == nullptr) {
        return false;
    }

    if (mount(saveData) != 0) {
        forge_fs_unmountSaveData();
        return false;
    }

    return true;
}

extern "C" void forge_fs_unmountSaveData(void)
{
    const bool jobSafe = isJobSafe();
    const auto saveData = forge_singleton_getInstanceByName("sSavedata");
    if (saveData == nullptr) {
        return;
    }

    auto& mounts = mountCount(saveData);

    // Wasn't mounted in the first place
    if (mounts == 0) {
        return;
    }

    nn::fs::CommitSaveData(kSaveDataMountPoint);

    const bool threadSafe = jobSafe || isThreadSafe(saveData);
    const auto mutex = getMutex(saveData);

    if (threadSafe) {
        nn::os::LockMutex(mutex);
    }

    mounts -= 1;
    if (mounts == 0) {
        nn::fs::Unmount(kSaveDataMountPoint);
    }

    if (threadSafe) {
        nn::os::UnlockMutex(mutex);
    }
}

extern "C" const char* forge_fs_getSaveDataMountPoint(void)
{
    return kSaveDataMountPoint;
}

extern "C" const char* forge_fs_getRomFsMountPoint(void)
{
    return kRomFsMountPoint;
}

extern "C" bool forge_fs_fileExists(const char* path)
{
    nn::fs::FileHandle handle;
    if (nn::fs::OpenFile(&handle, path, nn::fs::OpenMode_Read).IsFailure()) {
        return false;
    }

    nn::fs::CloseFile(handle);
    return true;
}
