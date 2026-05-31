#include "graphics/imgui_impl_nvn.h"

#include <nn/os.h>
#include <nvn/nvn_Cpp.h>
#include <nvn/nvn_CppMethods.h>

#include <cstdlib>
#include <span>
#include <vector>

struct ImGui_ImplNVN_Data {
    nvn::Device* device;
    nvn::Queue* queue;

    // Swap Chain related
    std::span<nvn::Texture*> swapChainTextures;
    nvn::Format swapChainFormat;

    // Command Buffer Resources
    nvn::CommandBuffer cmdBuffer;
    nvn::MemoryPool cmdMemPool;
    void* cmdMemStorage;
    void* controlMem;

    // Shader Resources
    nvn::Program shaderProgram;
    nvn::MemoryPool shaderMemPool;
    void* shaderMemStorage;

    // Per Frame Geometry
    nvn::MemoryPool transientPool;
    void* transientStorage;
    std::vector<nvn::Buffer> vtxBuffers;
    std::vector<nvn::Buffer> idxBuffers;
    std::vector<nvn::Buffer> uniformBuffers;
    std::vector<size_t> vtxSizes;
    std::vector<size_t> idxSizes;

    // Font Texture
    nvn::Texture fontTexture;
    nvn::Sampler fontSampler;
    nvn::MemoryPool texturePoolMem;
    nvn::MemoryPool samplerPoolMem;
    nvn::TexturePool texturePool;
    nvn::SamplerPool samplerPool;
    int fontTexId;
    int fontSamplerId;
    nvn::TextureHandle fontHandle;

    bool initialized;
};

void* InitMemoryPool(nvn::Device* device, nvn::MemoryPool& pool, size_t size)
{
    constexpr size_t kAlignment = 0x1000;

    void* storage = aligned_alloc(kAlignment, size);
    if (storage == nullptr) {
        return nullptr;
    }

    nvn::MemoryPoolBuilder builder { };
    builder
        .SetDevice(device)
        .SetDefaults()
        .SetFlags(nvn::MemoryPoolFlags::CPU_UNCACHED | nvn::MemoryPoolFlags::GPU_CACHED)
        .SetStorage(storage, size);

    if (!pool.Initialize(&builder)) {
        free(storage);
        return nullptr;
    }

    return storage;
}

IMGUI_IMPL_API void ImGui_ImplNVN_Init(nvn::Device* device, nvn::Queue* queue)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(!io.BackendRendererUserData && "Already initialized a renderer backend!");

    io.BackendPlatformName = "Switch";
    io.BackendRendererName = "imgui_impl_nvn";
    io.IniFilename = nullptr;
    io.MouseDrawCursor = false;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    ImGui_ImplNVN_Data* data = new ImGui_ImplNVN_Data();
    io.BackendRendererUserData = (void*)data;

    data->device = device;
    data->queue = queue;
}

IMGUI_IMPL_API void ImGui_ImplNVN_Shutdown()
{
}