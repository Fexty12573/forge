#include "forge/log.h"
#include "forge/nn/fs.h"
#include "forge/version.h"
#include "forge/wrapper.h"

#include <switch/kernel/svc.h>

static nn::fs::FileHandle s_file;
static s64 s_offset;
static bool s_enabled;
static u8 s_mutex[FORGE_NNOS_MUTEX_SIZE] __attribute__((aligned(4)));

extern "C" void forge_log_openFile(void)
{
    // Use SDK file handles: ARM32 FileHandle is four bytes, not eight.
    auto r = nn::fs::MountSdCard("forge_log");
    if (r.IsFailure()) {
        svcOutputDebugString("Forge: SD log mount failed", 26);
        return;
    }

    constexpr auto path = "forge_log:/forge-loader.log";
    r = nn::fs::OpenFile(&s_file, path, nn::fs::OpenMode_Write | nn::fs::OpenMode_Append);
    if (r.IsFailure()) {
        r = nn::fs::CreateFile(path, 0);
        if (r.IsFailure()) {
            return;
        }
        r = nn::fs::OpenFile(&s_file, path, nn::fs::OpenMode_Write | nn::fs::OpenMode_Append);
        if (r.IsFailure()) {
            return;
        }
    }
    r = nn::fs::SetFileSize(s_file, 0);
    if (r.IsFailure()) {
        nn::fs::CloseFile(s_file);
        return;
    }

    forge_nnosInitializeMutex(s_mutex, false);
    s_enabled = true;
    forge_log_trace("Forge hardware source build %s", FORGE_VERSION);
}

extern "C" void forge_log_writeFile(const char* text, size_t size)
{
    if (!s_enabled) {
        return;
    }

    forge_nnosLockMutex(s_mutex);
    const nn::fs::WriteOption flush { 1 };
    auto r = nn::fs::WriteFile(s_file, s_offset, text, size, flush);
    if (r.IsSuccess()) {
        s_offset += size;
        r = nn::fs::WriteFile(s_file, s_offset, "\n", 1, flush);
        if (r.IsSuccess()) {
            s_offset++;
        }
    }
    forge_nnosUnlockMutex(s_mutex);
}
