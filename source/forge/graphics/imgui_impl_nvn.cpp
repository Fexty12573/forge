#include "graphics/imgui_impl_nvn.h"
#include "forge/input.h"
#include "forge/log.h"
#include "forge/nn/fs.h"
#include "forge/nn/os.h"

#include <nvn/nvn_Cpp.h>
#include <nvn/nvn_CppMethods.h>
#include <nvnTool/nvnTool_GlslcInterface.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <list>
#include <memory>
#include <queue>
#include <span>
#include <vector>

// Experimental hack to forcefully patch the shader binary to GPUcode 1.14.
// TODO: Remove once a binary can be compiled with actual v1.5-1.14 GPUcode.
#ifndef FORGE_NVN_SHADER_PATCH_MINOR
#define FORGE_NVN_SHADER_PATCH_MINOR 14
#endif

struct ImGui_ImplNVN_CmdMemChunk {
    nvn::MemoryPool pool;
    void* storage;
};

struct Mat4 {
    float m[4][4];
};

struct __aligned(64) ImGui_ImplNVN_UBO {
    Mat4 proj;
};

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
    std::list<ImGui_ImplNVN_CmdMemChunk> extraCmdMem;
    std::vector<void*> extraControlMem;

    // Shader Resources
    nvn::Program shaderProgram;
    nvn::ShaderData shaderData[2];
    nvn::MemoryPool shaderMemPool;
    void* shaderMemStorage;
    ImGui_ImplNVN_UBO ubo;
    nvn::VertexAttribState attribStates[3];
    nvn::VertexStreamState streamState;

    // Per Frame Geometry
    nvn::MemoryPool transientPool;
    void* transientStorage;
    std::vector<nvn::Buffer> vtxBuffers;
    std::vector<nvn::Buffer> idxBuffers;
    std::vector<nvn::Buffer> uniformBuffers;
    std::vector<size_t> vtxSizes;
    std::vector<size_t> idxSizes;

    // Font Texture
    nvn::Sampler sampler;
    nvn::MemoryPool texturePoolMem;
    nvn::MemoryPool samplerPoolMem;
    nvn::TexturePool texturePool;
    nvn::SamplerPool samplerPool;
    void* samplerPoolStorage;
    void* texturePoolStorage;
    int samplerSlot;
    std::queue<int> unusedTextureSlots;

    u64 lastTick;
    float appliedDpiScale;

    bool initialized;

    int AllocateTextureSlot()
    {
        if (unusedTextureSlots.empty()) {
            return -1;
        }

        const int slot = unusedTextureSlots.front();
        unusedTextureSlots.pop();

        return slot;
    }

    void FreeTextureSlot(int slot)
    {
        unusedTextureSlots.push(slot);
    }
};

struct ImGui_ImplNVN_Texture {
    nvn::Texture texture;
    nvn::MemoryPool pool;
    void* poolStorage;
    int descriptorSlot;
};

template <size_t Align>
size_t AlignUp(size_t value)
{
    return (value + Align - 1) & ~(Align - 1);
}

static void GetProjOrtho(Mat4& out, float l, float r, float b, float t, float near, float far)
{
    std::memset(&out, 0, sizeof(out));

    out.m[0][0] = 2.0f / (r - l);
    out.m[1][1] = 2.0f / (t - b);
    out.m[2][2] = -1.0f;
    out.m[3][0] = (r + l) / (l - r);
    out.m[3][1] = (t + b) / (b - t);
    out.m[3][2] = 0.0f;
    out.m[3][3] = 1.0f;
}

#if FORGE_NVN_SHADER_PATCH_MINOR
static int PatchShaderControlVersion(void* control, size_t size, u32 fromMinor, u32 targetMinor)
{
    const size_t scan = std::min<size_t>(size >= 8 ? size - 8 : 0, 256);
    auto* bytes = static_cast<u8*>(control);
    for (size_t i = 0; i + 8 <= scan + 8; ++i) {
        u32 a, b;
        std::memcpy(&a, bytes + i, 4);
        std::memcpy(&b, bytes + i + 4, 4);
        if (a == 1u && b == fromMinor) {
            std::memcpy(bytes + i + 4, &targetMinor, 4);
            forge_log_warn("Patched shader control version 1.%u -> 1.%u at +0x%zX", fromMinor, targetMinor, i + 4);
            return static_cast<int>(i + 4);
        }
    }

    forge_log_warn("Version stamp {1,%u} not found in control section (size=%zu)", fromMinor, size);
    return -1;
}
#endif

static void* InitMemoryPool(nvn::Device* device, nvn::MemoryPool& pool, size_t size, nvn::MemoryPoolFlags extraFlags = 0, size_t minAlignment = 0, bool gpuCached = true)
{
    constexpr size_t kAlignment = 0x1000;
    constexpr size_t kSizeAlignment = 0x1000;

    const size_t alignment = std::max(kAlignment, minAlignment);

    const size_t alignedSize = AlignUp<kSizeAlignment>(size);
    void* storage = aligned_alloc(alignment, alignedSize);
    if (storage == nullptr) {
        return nullptr;
    }

    const auto baseFlags = gpuCached ? nvn::MemoryPoolFlags::GPU_CACHED : 0;
    nvn::MemoryPoolBuilder builder { };
    builder
        .SetDevice(device)
        .SetDefaults()
        .SetFlags(baseFlags | extraFlags)
        .SetStorage(storage, alignedSize);

    if (!pool.Initialize(&builder)) {
        free(storage);
        return nullptr;
    }

    return storage;
}

static ImGui_ImplNVN_Data* GetBackendData()
{
    return static_cast<ImGui_ImplNVN_Data*>(ImGui::GetIO().BackendRendererUserData);
}

static void ImGui_ImplNVN_CmdMemCallback(nvn::CommandBuffer* cmdBuffer, nvn::CommandBufferMemoryEvent::Enum event, size_t minSize, void* userData)
{
    auto bd = static_cast<ImGui_ImplNVN_Data*>(userData);
    if (event == nvn::CommandBufferMemoryEvent::OUT_OF_COMMAND_MEMORY) {
        ImGui_ImplNVN_CmdMemChunk& chunk = bd->extraCmdMem.emplace_back();
        chunk.storage = InitMemoryPool(
            bd->device,
            chunk.pool,
            minSize,
            nvn::MemoryPoolFlags::CPU_UNCACHED | nvn::MemoryPoolFlags::GPU_UNCACHED,
            0,
            false); // GPU Uncached

        if (chunk.storage == nullptr) {
            forge_log_error("Failed to allocate command buffer memory");
            std::terminate();
        }

        cmdBuffer->AddCommandMemory(&chunk.pool, 0, minSize);
    } else { // OUT_OF_CONTROL_MEMORY
        void* ptr = aligned_alloc(8, minSize);
        if (ptr == nullptr) {
            forge_log_error("Failed to allocate command buffer memory");
            std::terminate();
        }

        cmdBuffer->AddControlMemory(ptr, minSize);
        bd->extraControlMem.push_back(ptr);
    }
}

static void CreateCommandBuffer(ImGui_ImplNVN_Data* bd)
{
    constexpr size_t kControlMemorySize = 0x10000; // 64K
    constexpr size_t kCommandMemorySize = 0x40000; // 256K

    if (!bd->cmdBuffer.Initialize(bd->device)) {
        forge_log_error("Failed to initialize command buffer");
        std::terminate();
    }

    bd->controlMem = aligned_alloc(8, kControlMemorySize);
    bd->cmdMemStorage = InitMemoryPool(
        bd->device,
        bd->cmdMemPool,
        kCommandMemorySize,
        nvn::MemoryPoolFlags::CPU_UNCACHED | nvn::MemoryPoolFlags::GPU_UNCACHED,
        0,
        false); // GPU Uncached

    if (bd->cmdMemStorage == nullptr || bd->controlMem == nullptr) {
        forge_log_error("Failed to allocate command buffer memory");
        std::terminate();
    }

    bd->cmdBuffer.AddControlMemory(bd->controlMem, kControlMemorySize);
    bd->cmdBuffer.AddCommandMemory(&bd->cmdMemPool, 0, kCommandMemorySize);

    bd->cmdBuffer.SetMemoryCallback(ImGui_ImplNVN_CmdMemCallback);
    bd->cmdBuffer.SetMemoryCallbackData(bd);
}

static void LoadShaders(ImGui_ImplNVN_Data* bd)
{
    constexpr auto kPrecompiledShaderPath = "app:/nativeNX/forge/shaders/compiled/imgui.fbin";

    nn::fs::FileHandle handle;
    if (nn::fs::OpenFile(&handle, kPrecompiledShaderPath, nn::fs::OpenMode_Read).IsFailure()) {
        forge_log_error("Shader binary missing! Make sure you installed all files correctly.");
        std::terminate();
    }

    s64 fileSize = 0;
    nn::fs::GetFileSize(&fileSize, handle);
    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(fileSize);

    if (nn::fs::ReadFile(handle, 0, buffer.get(), static_cast<size_t>(fileSize)).IsFailure()) {
        forge_log_error("Failed to read shader from file! Make sure you installed all files correctly.");
        std::terminate();
    }

    forge_log_info("Shader loaded (Size: %li)", fileSize);

    bd->shaderMemStorage = InitMemoryPool(
        bd->device,
        bd->shaderMemPool,
        fileSize,
        nvn::MemoryPoolFlags::CPU_UNCACHED | nvn::MemoryPoolFlags::SHADER_CODE);

    if (bd->shaderMemStorage == nullptr) {
        forge_log_error("Failed to allocate shader memory. Exiting");
        std::terminate();
    }

    constexpr size_t kDataAlignment = 0x100;
    const uint64_t shaderMemAddr = bd->shaderMemPool.GetBufferAddress();
    const auto ptr = static_cast<u8*>(bd->shaderMemPool.Map());
    size_t offset = 0;

    bool vertexFound = false, fragmentFound = false;

    const auto output = reinterpret_cast<GLSLCoutput*>(buffer.get());
    for (u32 i = 0; i < output->numSections; ++i) {
        if (output->headers[i].genericHeader.common.type == GLSLC_SECTION_TYPE_GPU_CODE) {
            const auto gpuCode = &output->headers[i].gpuCodeHeader;

            const char* data = (const char*)output + gpuCode->common.dataOffset;

            int dataIndex = -1;
            switch (gpuCode->stage) {
            case NVN_SHADER_STAGE_VERTEX:
                vertexFound = true;
                dataIndex = 0;
                break;
            case NVN_SHADER_STAGE_FRAGMENT:
                fragmentFound = true;
                dataIndex = 1;
                break;
            default:
                continue;
            }

            // Store control
            std::memcpy(ptr + offset, data + gpuCode->controlOffset, gpuCode->controlSize);
            bd->shaderData[dataIndex].control = ptr + offset;
#if FORGE_NVN_SHADER_PATCH_MINOR
            PatchShaderControlVersion(ptr + offset, gpuCode->controlSize, 16, FORGE_NVN_SHADER_PATCH_MINOR);
#endif
            offset += AlignUp<kDataAlignment>(gpuCode->controlSize);

            // Store data
            std::memcpy(ptr + offset, data + gpuCode->dataOffset, gpuCode->dataSize);
            bd->shaderData[dataIndex].data = shaderMemAddr + offset;
            offset += AlignUp<kDataAlignment>(gpuCode->dataSize);
        }
    }

    bd->shaderMemPool.FlushMappedRange(0, offset);

    // Check if the precompiled shader binary version is compatible with the driver
    {
        int maxMajor = 0, minMajor = 0, maxMinor = 0, minMinor = 0;
        bd->device->GetInteger(nvn::DeviceInfo::GLSLC_MAX_SUPPORTED_GPU_CODE_MAJOR_VERSION, &maxMajor);
        bd->device->GetInteger(nvn::DeviceInfo::GLSLC_MIN_SUPPORTED_GPU_CODE_MAJOR_VERSION, &minMajor);
        bd->device->GetInteger(nvn::DeviceInfo::GLSLC_MAX_SUPPORTED_GPU_CODE_MINOR_VERSION, &maxMinor);
        bd->device->GetInteger(nvn::DeviceInfo::GLSLC_MIN_SUPPORTED_GPU_CODE_MINOR_VERSION, &minMinor);
        const int binMajor = (int)output->versionInfo.gpuCodeVersionMajor;
        const int binMinor = (int)output->versionInfo.gpuCodeVersionMinor;
        forge_log_info(
            "Shader bin: api=%u.%u gpuCode=%d.%d pkg=%u | driver accepts gpuCode major[%d..%d] minor[%d..%d]",
            output->versionInfo.apiMajor, output->versionInfo.apiMinor,
            binMajor, binMinor, output->versionInfo.package,
            minMajor, maxMajor, minMinor, maxMinor);

        const bool inRange = binMajor >= minMajor && binMajor <= maxMajor
            && binMinor >= minMinor && binMinor <= maxMinor;
        if (!inRange) {
            forge_log_error(
                "Shader gpuCode %d.%d is outside the driver's accepted range (major[%d..%d] minor[%d..%d])",
                binMajor,
                binMinor,
                minMajor,
                maxMajor,
                minMinor,
                maxMinor);

#if !FORGE_NVN_SHADER_PATCH_MINOR
            // When the control-version patch is active the container still
            // reports the original (rejected) version here; let SetShaders be
            // the real arbiter instead of bailing out early.
            std::terminate();
#endif
        }
    }

    if (!vertexFound || !fragmentFound) {
        forge_log_error("Could not find either vertex or fragment stage in shader. Make sure you installed all files correctly.");
        std::terminate();
    }

    if (!bd->shaderProgram.Initialize(bd->device)) {
        forge_log_error("Failed to initialize shader program");
        std::terminate();
    }

    if (!bd->shaderProgram.SetShaders(2, bd->shaderData)) {
        forge_log_error("Failed to set shaders, rejected by NVN");
        std::terminate();
    }

    bd->shaderProgram.SetDebugLabel("[forge] ImGui Shader");

    // Shader Attributes
    bd->attribStates[0].SetDefaults().SetFormat(nvn::Format::RG32F, offsetof(ImDrawVert, pos));
    bd->attribStates[1].SetDefaults().SetFormat(nvn::Format::RG32F, offsetof(ImDrawVert, uv));
    bd->attribStates[2].SetDefaults().SetFormat(nvn::Format::RGBA8, offsetof(ImDrawVert, col));

    bd->streamState.SetDefaults().SetStride(sizeof(ImDrawVert));
}

static void CreateTransientMemory(ImGui_ImplNVN_Data* bd, size_t maxVertices = 0, size_t maxIndices = 0)
{
    constexpr size_t kUboAlignment = 0x100;
    constexpr size_t kBaseMaxVertices = 0x10000; // 64K
    constexpr size_t kBaseMaxIndices = 0x18000; // 96K

    if (bd->initialized) {
        // The GPU may still be reading these buffers from in-flight frames.
        // Wait for it to drain before tearing the pool down.
        bd->queue->Finish();

        for (auto& buf : bd->uniformBuffers) {
            buf.Finalize();
        }

        for (auto& buf : bd->vtxBuffers) {
            buf.Finalize();
        }

        for (auto& buf : bd->idxBuffers) {
            buf.Finalize();
        }

        bd->transientPool.Finalize();
        free(bd->transientStorage);
        bd->transientStorage = nullptr;
    }

    const auto framesInFlight = bd->swapChainTextures.size();
    maxVertices = maxVertices ? maxVertices : kBaseMaxVertices;
    maxIndices = maxIndices ? maxIndices : kBaseMaxIndices;

    const auto uboSizePerFrame = AlignUp<kUboAlignment>(sizeof(Mat4));
    const auto vtxSizePerFrame = AlignUp<kUboAlignment>(maxVertices * sizeof(ImDrawVert));
    const auto idxSizePerFrame = AlignUp<kUboAlignment>(maxIndices * sizeof(ImDrawIdx));

    const auto transientSizePerFrame = uboSizePerFrame + vtxSizePerFrame + idxSizePerFrame;
    const auto transientSize = transientSizePerFrame * framesInFlight;

    bd->transientStorage = InitMemoryPool(
        bd->device,
        bd->transientPool,
        transientSize,
        nvn::MemoryPoolFlags::CPU_CACHED);

    if (bd->transientStorage == nullptr) {
        forge_log_error("Failed to allocate memory for transient pool. Exiting");
        std::terminate();
    }

    for (size_t i = 0; i < framesInFlight; ++i) {
        auto offset = i * transientSizePerFrame;

        bd->vtxSizes[i] = maxVertices;
        bd->idxSizes[i] = maxIndices;

        nvn::BufferBuilder builder;
        builder.SetDefaults()
            .SetDevice(bd->device)
            .SetStorage(&bd->transientPool, offset, uboSizePerFrame);

        bd->uniformBuffers[i].Initialize(&builder);
        offset += uboSizePerFrame;

        builder.SetDefaults()
            .SetDevice(bd->device)
            .SetStorage(&bd->transientPool, offset, vtxSizePerFrame);

        bd->vtxBuffers[i].Initialize(&builder);
        offset += vtxSizePerFrame;

        builder.SetDefaults()
            .SetDevice(bd->device)
            .SetStorage(&bd->transientPool, offset, idxSizePerFrame);

        bd->idxBuffers[i].Initialize(&builder);
        offset += idxSizePerFrame;
    }
}

static void DestroyTexture(ImTextureData* tex)
{
    auto bd = GetBackendData();

    if (const auto backendTex = static_cast<ImGui_ImplNVN_Texture*>(tex->BackendUserData)) {
        backendTex->texture.Finalize();
        backendTex->pool.Finalize();
        free(backendTex->poolStorage);
        bd->FreeTextureSlot(backendTex->descriptorSlot);
        IM_DELETE(backendTex);

        tex->SetTexID(ImTextureID_Invalid);
        tex->BackendUserData = nullptr;
    }

    tex->SetStatus(ImTextureStatus_Destroyed);
}

static void UpdateTexture(ImTextureData* tex)
{
    auto bd = GetBackendData();

    if (tex->Status == ImTextureStatus_WantCreate) {
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
        const auto pixels = static_cast<u32*>(tex->GetPixels());
        ImGui_ImplNVN_Texture* backendTex = IM_NEW(ImGui_ImplNVN_Texture)();

        nvn::TextureBuilder builder;
        builder.SetDevice(bd->device)
            .SetDefaults()
            .SetFormat(nvn::Format::RGBA8)
            .SetTarget(nvn::TextureTarget::TARGET_2D)
            .SetSize2D(tex->Width, tex->Height);

        backendTex->poolStorage = InitMemoryPool(
            bd->device,
            backendTex->pool,
            builder.GetStorageSize(),
            nvn::MemoryPoolFlags::CPU_CACHED,
            builder.GetStorageAlignment());

        if (backendTex->poolStorage == nullptr) {
            forge_log_error("Failed to allocate texture pool storage");
            std::terminate();
        }

        builder.SetStorage(&backendTex->pool, 0);

        if (!backendTex->texture.Initialize(&builder)) {
            forge_log_error("Failed to initialize font texture");
            std::terminate();
        }

        const nvn::CopyRegion region {
            .xoffset = 0,
            .yoffset = 0,
            .zoffset = 0,
            .width = backendTex->texture.GetWidth(),
            .height = backendTex->texture.GetHeight(),
            .depth = 1,
        };

        backendTex->texture.WriteTexels(nullptr, &region, pixels);
        backendTex->texture.FlushTexels(nullptr, &region);

        backendTex->descriptorSlot = bd->AllocateTextureSlot();
        if (backendTex->descriptorSlot == -1) {
            forge_log_error("Insufficient number of texture descriptors. Report this to the developer");
            std::terminate();
        }

        bd->texturePool.RegisterTexture(backendTex->descriptorSlot, &backendTex->texture, nullptr);

        tex->SetTexID(bd->device->GetTextureHandle(backendTex->descriptorSlot, bd->samplerSlot));
        tex->SetStatus(ImTextureStatus_OK);
        tex->BackendUserData = backendTex;
    } else if (tex->Status == ImTextureStatus_WantUpdates) {
        const auto backendTex = static_cast<ImGui_ImplNVN_Texture*>(tex->BackendUserData);

        for (const auto& r : tex->Updates) {
            const nvn::CopyRegion region {
                .xoffset = r.x,
                .yoffset = r.y,
                .zoffset = 0,
                .width = r.w,
                .height = r.h,
                .depth = 1,
            };

            backendTex->texture.WriteTexels(nullptr, &region, tex->GetPixelsAt(r.x, r.y));
        }

        const nvn::CopyRegion boundingBox {
            .xoffset = tex->UpdateRect.x,
            .yoffset = tex->UpdateRect.y,
            .zoffset = 0,
            .width = tex->UpdateRect.w,
            .height = tex->UpdateRect.h,
            .depth = 1,
        };

        backendTex->texture.FlushTexels(nullptr, &boundingBox);
    } else if (tex->Status == ImTextureStatus_WantDestroy && tex->UnusedFrames > 0) {
        DestroyTexture(tex);
    }
}

static void CreateTextureResources(ImGui_ImplNVN_Data* bd)
{
    constexpr int kNumSamplerDescriptors = 1;
    constexpr int kNumTextureDescriptors = 128;

    // Get Parameters
    int reservedSamplerSlots = 0, reservedTextureSlots = 0;
    bd->device->GetInteger(nvn::DeviceInfo::RESERVED_SAMPLER_DESCRIPTORS, &reservedSamplerSlots);
    bd->device->GetInteger(nvn::DeviceInfo::RESERVED_TEXTURE_DESCRIPTORS, &reservedTextureSlots);

    int samplerDescriptorSize = 0, textureDescriptorSize = 0;
    bd->device->GetInteger(nvn::DeviceInfo::SAMPLER_DESCRIPTOR_SIZE, &samplerDescriptorSize);
    bd->device->GetInteger(nvn::DeviceInfo::TEXTURE_DESCRIPTOR_SIZE, &textureDescriptorSize);

    // Sampler Pool
    const auto samplerDescriptorCount = reservedSamplerSlots + kNumSamplerDescriptors;
    const auto samplerPoolSize = samplerDescriptorCount * samplerDescriptorSize;
    bd->samplerPoolStorage = InitMemoryPool(
        bd->device,
        bd->samplerPoolMem,
        static_cast<size_t>(samplerPoolSize),
        nvn::MemoryPoolFlags::CPU_UNCACHED);

    if (bd->samplerPoolStorage == nullptr) {
        forge_log_error("Failed to allocate sampler pool storage");
        std::terminate();
    }

    if (!bd->samplerPool.Initialize(&bd->samplerPoolMem, 0, samplerDescriptorCount)) {
        forge_log_error("Failed to initialize sampler pool");
        std::terminate();
    }

    nvn::SamplerBuilder builder;
    builder.SetDevice(bd->device)
        .SetDefaults()
        .SetMinMagFilter(nvn::MinFilter::LINEAR, nvn::MagFilter::LINEAR)
        .SetWrapMode(nvn::WrapMode::CLAMP, nvn::WrapMode::CLAMP, nvn::WrapMode::CLAMP);

    if (!bd->sampler.Initialize(&builder)) {
        forge_log_error("Failed to initialize font sampler");
        std::terminate();
    }

    bd->samplerSlot = reservedSamplerSlots;
    bd->samplerPool.RegisterSampler(bd->samplerSlot, &bd->sampler);

    // Texture Pool
    const auto textureDescriptorCount = reservedTextureSlots + kNumTextureDescriptors;
    const auto texturePoolSize = textureDescriptorCount * textureDescriptorSize;
    bd->texturePoolStorage = InitMemoryPool(
        bd->device,
        bd->texturePoolMem,
        static_cast<size_t>(texturePoolSize),
        nvn::MemoryPoolFlags::CPU_UNCACHED);

    if (bd->texturePoolStorage == nullptr) {
        forge_log_error("Failed to allocate texture pool storage");
        std::terminate();
    }

    if (!bd->texturePool.Initialize(&bd->texturePoolMem, 0, textureDescriptorCount)) {
        forge_log_error("Failed to initialize texture pool");
        std::terminate();
    }

    for (int i = reservedTextureSlots; i < textureDescriptorCount; ++i) {
        bd->unusedTextureSlots.push(i);
    }
}

IMGUI_IMPL_API void ImGui_ImplNVN_Init(nvn::Device* device, nvn::Queue* queue, std::span<nvn::Texture*> swapChainTextures)
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
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    ImGui_ImplNVN_Data* bd = IM_NEW(ImGui_ImplNVN_Data)();
    io.BackendRendererUserData = bd;

    bd->device = device;
    bd->queue = queue;
    bd->swapChainTextures = swapChainTextures;
    bd->lastTick = 0;
    bd->appliedDpiScale = 1.0f; // style metrics start at the default (1x) scale

    // nvn::Buffer is non-copyable/non-movable, so resize() won't compile.
    // Move-assigning a freshly sized vector swaps the storage instead.
    const auto framesInFlight = swapChainTextures.size();
    bd->uniformBuffers = std::vector<nvn::Buffer>(framesInFlight);
    bd->vtxBuffers = std::vector<nvn::Buffer>(framesInFlight);
    bd->idxBuffers = std::vector<nvn::Buffer>(framesInFlight);
    bd->vtxSizes.resize(framesInFlight);
    bd->idxSizes.resize(framesInFlight);

    LoadShaders(bd);
    CreateTransientMemory(bd);
    CreateTextureResources(bd);
    CreateCommandBuffer(bd);

    bd->initialized = true;
}

IMGUI_IMPL_API void ImGui_ImplNVN_Shutdown()
{
    // TODO: Deallocate and finalize everything
}

static void ImGui_ImplNVN_UpdateGamepadNav()
{
    ImGuiIO& io = ImGui::GetIO();
    if (!(io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) || !forge_input_isConnected())
        return;

    auto btn = [&](ImGuiKey key, ForgeButton b) { io.AddKeyEvent(key, forge_input_isDown(b)); };
    btn(ImGuiKey_GamepadFaceDown, ForgeButton_A); // Nintendo A = activate
    btn(ImGuiKey_GamepadFaceRight, ForgeButton_B); // Nintendo B = cancel
    btn(ImGuiKey_GamepadFaceUp, ForgeButton_X);
    btn(ImGuiKey_GamepadFaceLeft, ForgeButton_Y);
    btn(ImGuiKey_GamepadDpadLeft, ForgeButton_Left);
    btn(ImGuiKey_GamepadDpadRight, ForgeButton_Right);
    btn(ImGuiKey_GamepadDpadUp, ForgeButton_Up);
    btn(ImGuiKey_GamepadDpadDown, ForgeButton_Down);
    btn(ImGuiKey_GamepadL1, ForgeButton_L);
    btn(ImGuiKey_GamepadR1, ForgeButton_R);
    btn(ImGuiKey_GamepadStart, ForgeButton_Plus);
    btn(ImGuiKey_GamepadBack, ForgeButton_Minus);

    // Left stick -> analog nav (with a small deadzone)
    float lx, ly;
    forge_input_getStickL(&lx, &ly);
    auto axis = [&](ImGuiKey key, float v) {
        v = std::abs(v) > 0.10f ? v : 0.0f;
        io.AddKeyAnalogEvent(key, v != 0.0f, std::abs(v));
    };

    axis(ImGuiKey_GamepadLStickLeft, lx < 0 ? lx : 0.0f);
    axis(ImGuiKey_GamepadLStickRight, lx > 0 ? lx : 0.0f);
    axis(ImGuiKey_GamepadLStickUp, ly > 0 ? ly : 0.0f);
    axis(ImGuiKey_GamepadLStickDown, ly < 0 ? ly : 0.0f);
}

static void ImGui_ImplNVN_UpdateMouseFromTouch()
{
    ImGuiIO& io = ImGui::GetIO();

    float tx, ty;
    if (forge_input_getTouch(&tx, &ty)) {
        io.AddMousePosEvent(
            tx * io.DisplaySize.x / 1280.0f,
            ty * io.DisplaySize.y / 720.0f);

        io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
    } else {
        io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
    }
}

static void ImGui_ImplNVN_UpdateKeyboard()
{
    ImGuiIO& io = ImGui::GetIO();

    io.AddKeyEvent(ImGuiMod_Ctrl, forge_input_isCtrlDown());
    io.AddKeyEvent(ImGuiMod_Shift, forge_input_isShiftDown());
    io.AddKeyEvent(ImGuiMod_Alt, forge_input_isAltDown());

    static const struct {
        ForgeKey k;
        ImGuiKey key;
    } kMap[] = {
        { ForgeKey_Tab, ImGuiKey_Tab },
        { ForgeKey_Left, ImGuiKey_LeftArrow },
        { ForgeKey_Right, ImGuiKey_RightArrow },
        { ForgeKey_Up, ImGuiKey_UpArrow },
        { ForgeKey_Down, ImGuiKey_DownArrow },
        { ForgeKey_PageUp, ImGuiKey_PageUp },
        { ForgeKey_PageDown, ImGuiKey_PageDown },
        { ForgeKey_Home, ImGuiKey_Home },
        { ForgeKey_End, ImGuiKey_End },
        { ForgeKey_Insert, ImGuiKey_Insert },
        { ForgeKey_Delete, ImGuiKey_Delete },
        { ForgeKey_Backspace, ImGuiKey_Backspace },
        { ForgeKey_Space, ImGuiKey_Space },
        { ForgeKey_Enter, ImGuiKey_Enter },
        { ForgeKey_Escape, ImGuiKey_Escape },
    };
    for (auto& m : kMap) {
        io.AddKeyEvent(m.key, forge_input_isKeyDown(m.k));
    }

    // Contiguous ranges: A–Z and digits.
    for (int i = 0; i < 26; ++i) {
        io.AddKeyEvent((ImGuiKey)(ImGuiKey_A + i), forge_input_isKeyDown((ForgeKey)(ForgeKey_A + i)));
    }
    for (int i = 0; i < 9; ++i) {
        io.AddKeyEvent((ImGuiKey)(ImGuiKey_1 + i), forge_input_isKeyDown((ForgeKey)(ForgeKey_1 + i)));
    }

    io.AddKeyEvent(ImGuiKey_0, forge_input_isKeyDown(ForgeKey_0));

    // Text input (basic US-QWERTY): one char per key pressed this frame.
    const bool shift = forge_input_isShiftDown();
    for (int i = 0; i < 26; ++i) {
        if (forge_input_isKeyPressed((ForgeKey)(ForgeKey_A + i))) {
            io.AddInputCharacter((shift ? 'A' : 'a') + i);
        }
    }

    if (forge_input_isKeyPressed(ForgeKey_Space)) {
        io.AddInputCharacter(' ');
    }

    static const char *kDigits = "1234567890", *kShifted = "!@#$%^&*()";
    for (int i = 0; i < 9; ++i) {
        if (forge_input_isKeyPressed((ForgeKey)(ForgeKey_1 + i))) {
            io.AddInputCharacter(shift ? kShifted[i] : kDigits[i]);
        }
    }

    if (forge_input_isKeyPressed(ForgeKey_0)) {
        io.AddInputCharacter(shift ? ')' : '0');
    }
}

IMGUI_IMPL_API void ImGui_ImplNVN_NewFrame()
{
    const auto bd = GetBackendData();
    IM_ASSERT(bd != nullptr && "Backend not initialized, did you call ImGui_ImplNVN_Init()?");

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = {
        static_cast<float>(bd->swapChainTextures[0]->GetWidth()),
        static_cast<float>(bd->swapChainTextures[0]->GetHeight())
    };

    io.DisplayFramebufferScale = ImVec2(1, 1);

    // DPI scaling
    ImGuiStyle& style = ImGui::GetStyle();
    const float dpiScale = io.DisplaySize.y / 720.0f;
    if (dpiScale != bd->appliedDpiScale && bd->appliedDpiScale > 0.0f) {
        style.ScaleAllSizes(dpiScale / bd->appliedDpiScale);
        bd->appliedDpiScale = dpiScale;
    }
    style.FontScaleDpi = dpiScale;

    const auto nowTick = nn::os::GetSystemTick().value;
    const auto freq = nn::os::GetSystemTickFrequency();

    io.DeltaTime = bd->lastTick == 0
        ? 1 / 60.0f // Avoid weird initial delta time
        : (nowTick - bd->lastTick) / static_cast<float>(freq);

    bd->lastTick = nowTick;

    forge_input_update();

    ImGui_ImplNVN_UpdateGamepadNav();
    ImGui_ImplNVN_UpdateMouseFromTouch();
    ImGui_ImplNVN_UpdateKeyboard();
}

IMGUI_IMPL_API void ImGui_ImplNVN_SetSwapChainTextures(std::span<nvn::Texture*> swapChainTextures)
{
    const auto bd = GetBackendData();
    bd->swapChainTextures = swapChainTextures;
}

static void SetupRenderState(ImGui_ImplNVN_Data* bd, int textureIndex)
{
    bd->cmdBuffer.BindProgram(&bd->shaderProgram, nvn::ShaderStageBits::VERTEX | nvn::ShaderStageBits::FRAGMENT);

    bd->cmdBuffer.BindUniformBuffer(
        nvn::ShaderStage::VERTEX,
        0,
        bd->uniformBuffers[textureIndex].GetAddress(),
        sizeof(ImGui_ImplNVN_UBO));

    bd->cmdBuffer.UpdateUniformBuffer(
        bd->uniformBuffers[textureIndex].GetAddress(),
        sizeof(ImGui_ImplNVN_UBO),
        0,
        sizeof(ImGui_ImplNVN_UBO),
        &bd->ubo);

    nvn::PolygonState ps;
    ps.SetDefaults()
        .SetPolygonMode(nvn::PolygonMode::FILL)
        .SetCullFace(nvn::Face::NONE)
        .SetFrontFace(nvn::FrontFace::CCW);

    bd->cmdBuffer.BindPolygonState(&ps);

    nvn::ColorState cs;
    cs.SetDefaults()
        .SetLogicOp(nvn::LogicOp::COPY)
        .SetAlphaTest(nvn::AlphaFunc::ALWAYS);

    for (int i = 0; i < 8; ++i) {
        cs.SetBlendEnable(i, true);
    }

    bd->cmdBuffer.BindColorState(&cs);

    nvn::BlendState bs;
    bs.SetDefaults()
        .SetBlendFunc(
            nvn::BlendFunc::SRC_ALPHA,
            nvn::BlendFunc::ONE_MINUS_SRC_ALPHA,
            nvn::BlendFunc::ONE,
            nvn::BlendFunc::ZERO)
        .SetBlendEquation(nvn::BlendEquation::ADD, nvn::BlendEquation::ADD);

    bd->cmdBuffer.BindBlendState(&bs);

    nvn::DepthStencilState dss;
    dss.SetDefaults()
        .SetDepthTestEnable(false)
        .SetDepthWriteEnable(false);

    bd->cmdBuffer.BindDepthStencilState(&dss);

    bd->cmdBuffer.BindVertexAttribState(3, bd->attribStates);
    bd->cmdBuffer.BindVertexStreamState(1, &bd->streamState);

    bd->cmdBuffer.SetSamplerPool(&bd->samplerPool);
    bd->cmdBuffer.SetTexturePool(&bd->texturePool);

    const auto rt = bd->swapChainTextures[textureIndex];
    bd->cmdBuffer.SetRenderTargets(1, &rt, nullptr, nullptr, nullptr);
    bd->cmdBuffer.SetViewport(0, 0, rt->GetWidth(), rt->GetHeight());
}

IMGUI_IMPL_API void ImGui_ImplNVN_RenderDrawData(nvn::Queue* queue, ImDrawData* drawData, int textureIndex)
{
    // Avoid rendering when minimized
    if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) {
        return;
    }

    const auto bd = GetBackendData();

    if (drawData->Textures != nullptr) {
        for (const auto tex : *drawData->Textures) {
            if (tex->Status != ImTextureStatus_OK) {
                UpdateTexture(tex);
            }
        }
    }

    const auto totalVtxCount = static_cast<size_t>(drawData->TotalVtxCount);
    const auto totalIdxCount = static_cast<size_t>(drawData->TotalIdxCount);

    if (bd->vtxSizes[textureIndex] < totalVtxCount || bd->idxSizes[textureIndex] < totalIdxCount) {
        constexpr size_t kVertexHeadroom = 5000;
        constexpr size_t kIndexHeadroom = 10000;
        CreateTransientMemory(bd, totalVtxCount + kVertexHeadroom, totalIdxCount + kIndexHeadroom);
    }

    GetProjOrtho(bd->ubo.proj, 0.0f, drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, -1.0f, 1.0f);

    bd->cmdBuffer.BeginRecording();

    SetupRenderState(bd, textureIndex);

    bd->cmdBuffer.BindVertexBuffer(
        0,
        bd->vtxBuffers[textureIndex].GetAddress(),
        totalVtxCount * sizeof(ImDrawVert));

    auto vtxDst = static_cast<ImDrawVert*>(bd->vtxBuffers[textureIndex].Map());
    auto idxDst = static_cast<ImDrawIdx*>(bd->idxBuffers[textureIndex].Map());
    for (const auto drawList : drawData->CmdLists) {
        std::memcpy(vtxDst, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(idxDst, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtxDst += drawList->VtxBuffer.Size;
        idxDst += drawList->IdxBuffer.Size;
    }

    bd->vtxBuffers[textureIndex].FlushMappedRange(0, totalVtxCount * sizeof(ImDrawVert));
    bd->idxBuffers[textureIndex].FlushMappedRange(0, totalIdxCount * sizeof(ImDrawIdx));

    const ImVec2 clipOff = drawData->DisplayPos;
    const ImVec2 clipScale = drawData->FramebufferScale;
    const auto rt = bd->swapChainTextures[textureIndex];

    for (const auto drawList : drawData->CmdLists) {
        for (const auto& cmd : drawList->CmdBuffer) {
            if (cmd.UserCallback != nullptr) {
                cmd.UserCallback(drawList, &cmd);
            } else {
                const ImVec2 clipMin {
                    std::clamp((cmd.ClipRect.x - clipOff.x) * clipScale.x, 0.0f, static_cast<float>(rt->GetWidth())),
                    std::clamp((cmd.ClipRect.y - clipOff.y) * clipScale.y, 0.0f, static_cast<float>(rt->GetHeight()))
                };
                const ImVec2 clipMax {
                    (cmd.ClipRect.z - clipOff.x) * clipScale.x,
                    (cmd.ClipRect.w - clipOff.y) * clipScale.y
                };

                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
                    continue;
                }

                bd->cmdBuffer.SetScissor(
                    clipMin.x,
                    clipMin.y,
                    clipMax.x - clipMin.x,
                    clipMax.y - clipMin.y);

                bd->cmdBuffer.BindTexture(
                    nvn::ShaderStage::FRAGMENT,
                    0,
                    cmd.GetTexID());

                const auto idxBufferAddr = bd->idxBuffers[textureIndex].GetAddress();

                bd->cmdBuffer.DrawElementsBaseVertex(
                    nvn::DrawPrimitive::TRIANGLES,
                    sizeof(ImDrawIdx) == 2 ? nvn::IndexType::UNSIGNED_SHORT : nvn::IndexType::UNSIGNED_INT,
                    static_cast<int>(cmd.ElemCount),
                    idxBufferAddr + (cmd.IdxOffset * sizeof(ImDrawIdx)),
                    cmd.VtxOffset);
            }
        }
    }

    const auto handle = bd->cmdBuffer.EndRecording();
    queue->SubmitCommands(1, &handle);
    queue->Flush();
}
