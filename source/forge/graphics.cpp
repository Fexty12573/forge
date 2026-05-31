#include "forge/graphics.h"
#include "forge/hook.h"
#include "forge/log.h"
#include "forge/nn/ro.h"

#include <nvn/nvn.h>

#include <string_view>

static Hook s_nvnBoostrapperLoaderHook;
static PFNNVNGENERICFUNCPTRPROC (*s_originalNvnBootstrapLoader)(const char*) = nullptr;
static PFNNVNDEVICEGETPROCADDRESSPROC s_defaultNvnGetProcAddress = nullptr;
static PFNNVNDEVICEINITIALIZEPROC s_defaultNvnDeviceInitialize = nullptr;

static void* nvnBootstrapLoaderHook(const char* name);
static void* customDeviceGetProcAddress(const NVNdevice* device, const char* name);
static NVNboolean customDeviceInitialize(NVNdevice* device, NVNdeviceBuilder* builder);

extern "C" bool forge_graphics_init(void)
{
    uintptr_t func;
    const auto result = nn::ro::LookupSymbol(&func, "nvnBootstrapLoader");
    if (func == 0 || R_FAILED(result)) {
        return false;
    }

    forge_log_trace("[forge] Found nvnBootstrapLoader at 0x%X", func);

    s_nvnBoostrapperLoaderHook = forge_hook_create(
        (void*)func,
        (void*)nvnBootstrapLoaderHook,
        (void**)&s_originalNvnBootstrapLoader);

    if (s_originalNvnBootstrapLoader == NULL) {
        return false;
    }

    return true;
}

void* nvnBootstrapLoaderHook(const char* name)
{
    using namespace std::string_view_literals;

    const auto result = s_originalNvnBootstrapLoader(name);

    if (name == "nvnDeviceGetProcAddress"sv) {
        forge_log_trace("[forge] nvnDeviceGetProcAddress requested");
        s_defaultNvnGetProcAddress = (PFNNVNDEVICEGETPROCADDRESSPROC)result;
        return (void*)customDeviceGetProcAddress;
    }

    return (void*)result;
}

void* customDeviceGetProcAddress(const NVNdevice* device, const char* name)
{
    using namespace std::string_view_literals;

    const auto result = s_defaultNvnGetProcAddress(device, name);

    if (name == "nvnDeviceInitialize"sv) {
        forge_log_trace("[forge] nvnDeviceInitialize requested");
        s_defaultNvnDeviceInitialize = (PFNNVNDEVICEINITIALIZEPROC)result;
        return (void*)customDeviceInitialize;
    }

    return (void*)result;
}

NVNboolean customDeviceInitialize(NVNdevice* device, NVNdeviceBuilder* builder)
{
    forge_log_trace("[forge] nvnDeviceInitialize called");
    return s_defaultNvnDeviceInitialize(device, builder);
}
