#include "forge/graphics.h"
#include "forge/hook.h"
#include "forge/log.h"
#include "forge/nn/ro.h"

#include <nvn/nvn.h>
#include <nvn/nvn_Cpp.h>
#include <nvn/nvn_CppFuncPtrBase.h>
#include <nvn/nvn_CppMethods.h>

#include <memory>
#include <string_view>
#include <vector>

static Hook s_nvnBoostrapperLoaderHook;
static PFNNVNGENERICFUNCPTRPROC (*s_originalNvnBootstrapLoader)(const char*) = nullptr;
static PFNNVNDEVICEGETPROCADDRESSPROC s_defaultNvnGetProcAddress = nullptr;
static PFNNVNDEVICEINITIALIZEPROC s_defaultNvnDeviceInitialize = nullptr;
static PFNNVNQUEUEINITIALIZEPROC s_defaultNvnQueueInitialize = nullptr;
static PFNNVNWINDOWBUILDERSETTEXTURESPROC s_defaultNvnWindowBuilderSetTextures = nullptr;
static PFNNVNQUEUEPRESENTTEXTUREPROC s_defaultNvnQueuePresentTexture = nullptr;

static void* nvnBootstrapLoaderHook(const char* name);
static void* customDeviceGetProcAddress(const NVNdevice* device, const char* name);
static NVNboolean customDeviceInitialize(NVNdevice* device, const NVNdeviceBuilder* builder);
static NVNboolean customQueueInitialize(NVNqueue* queue, const NVNqueueBuilder* builder);
static void customWindowBuilderSetTextures(NVNwindowBuilder* builder, int numTextures, NVNtexture* const* textures);
static void customQueuePresentTexture(NVNqueue* queue, NVNwindow* window, int textureIndex);

class Graphics {
public:
    static Graphics* get() { return s_instance; }

    static void createInstance()
    {
        s_instance = new Graphics;
    }

    void setDevice(NVNdevice* device)
    {
        m_device = (nvn::Device*)device;
    }

    void setQueue(NVNqueue* queue)
    {
        m_queue = (nvn::Queue*)queue;
    }

    void setSwapChainTextures(NVNtexture* const* textures, int numTextures)
    {
        m_swapChainTextures.clear();
        for (int i = 0; i < numTextures; i++) {
            m_swapChainTextures.push_back((nvn::Texture*)textures[i]);
        }
    }

    void initialize();
    bool isInitialized() const { return m_initialized; }

private:
    nvn::Device* m_device { nullptr };
    nvn::Queue* m_queue { nullptr };
    std::vector<nvn::Texture*> m_swapChainTextures { };
    bool m_initialized { false };

    static inline Graphics* s_instance = nullptr;
};

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

    Graphics::createInstance();

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
        s_defaultNvnDeviceInitialize = (PFNNVNDEVICEINITIALIZEPROC)result;
        return (void*)customDeviceInitialize;
    } else if (name == "nvnQueueInitialize"sv) {
        s_defaultNvnQueueInitialize = (PFNNVNQUEUEINITIALIZEPROC)result;
        return (void*)customQueueInitialize;
    } else if (name == "nvnWindowBuilderSetTextures"sv) {
        s_defaultNvnWindowBuilderSetTextures = (PFNNVNWINDOWBUILDERSETTEXTURESPROC)result;
        return (void*)customWindowBuilderSetTextures;
    } else if (name == "nvnQueuePresentTexture"sv) {
        s_defaultNvnQueuePresentTexture = (PFNNVNQUEUEPRESENTTEXTUREPROC)result;
        return (void*)customQueuePresentTexture;
    }

    return (void*)result;
}

NVNboolean customDeviceInitialize(NVNdevice* device, const NVNdeviceBuilder* builder)
{
    forge_log_trace("[forge] Obtained NVNdevice");

    Graphics::get()->setDevice(device);

    return s_defaultNvnDeviceInitialize(device, builder);
}

NVNboolean customQueueInitialize(NVNqueue* queue, const NVNqueueBuilder* builder)
{
    forge_log_trace("[forge] Obtained NVNqueue");

    Graphics::get()->setQueue(queue);

    return s_defaultNvnQueueInitialize(queue, builder);
}

void customWindowBuilderSetTextures(NVNwindowBuilder* builder, int numTextures, NVNtexture* const* textures)
{
    forge_log_trace("[forge] Obtained Swapchain Textures");

    Graphics::get()->setSwapChainTextures(textures, numTextures);

    return s_defaultNvnWindowBuilderSetTextures(builder, numTextures, textures);
}

void customQueuePresentTexture(NVNqueue* queue, NVNwindow* window, int textureIndex)
{
    auto gfx = Graphics::get();

    // The present queue is the one we must submit our overlay on; it is
    // authoritative here regardless of how many queues the game created.
    gfx->setQueue(queue);

    if (!gfx->isInitialized()) {
        gfx->initialize();
    }

    return s_defaultNvnQueuePresentTexture(queue, window, textureIndex);
}

void Graphics::initialize()
{
    nvn::nvnLoadCPPProcs(m_device, (nvn::DeviceGetProcAddressFunc)s_defaultNvnGetProcAddress);
}
