#include "graphics/imgui_impl_nvn.h"

#include <nn/os.h>
#include <nvn/nvn_Cpp.h>
#include <nvn/nvn_CppMethods.h>

#include <span>
#include <vector>

struct ImGui_ImplNVN_Data {
    nvn::Device* device;
    nvn::Queue* queue;

    // Swap Chain related
    std::span<nvn::Texture*> swapChainTextures;
    nvn::Format swapChainFormat;

    // Builders
    nvn::BufferBuilder bufferBuilder;
    nvn::MemoryPoolBuilder memPoolBuilder;
    nvn::TextureBuilder textureBuilder;
    nvn::SamplerBuilder samplerBuilder;

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

IMGUI_IMPL_API void ImGui_ImplNVN_Init()
{
}

IMGUI_IMPL_API void ImGui_ImplNVN_Shutdown()
{
}