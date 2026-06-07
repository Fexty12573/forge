#include "graphics/allocator.h"
#include "forge/log.h"

#include <nvn/nvn_CppMethods.h>

#include <algorithm>

static constexpr size_t kCmdCommandMemorySize = 0x40000; // 256K
static constexpr size_t kCmdControlMemorySize = 0x10000; // 64K

static constexpr size_t kCommandMemChunkSize = kCmdCommandMemorySize / 8;
static constexpr size_t kControlMemChunkSize = kCmdControlMemorySize / 8;

void* InitMemoryPool(nvn::Device* device, nvn::MemoryPool& pool, size_t size, nvn::MemoryPoolFlags extraFlags, size_t minAlignment, bool gpuCached)
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

static void MemoryCallback(nvn::CommandBuffer* cmdBuffer, nvn::CommandBufferMemoryEvent::Enum event, size_t minSize, void* userData)
{
    auto allocator = static_cast<CommandBufferAllocator*>(userData);
    allocator->AllocationCallback(cmdBuffer, event, minSize);
}

CommandBufferAllocator::CommandBufferAllocator(nvn::Device* device, nvn::CommandBuffer* cmdBuffer)
    : device(device)
    , cmdBuffer(cmdBuffer)
{
    // Command memory is CPU-written / GPU-read and NVN does not auto-flush it,
    // so it must be uncached (matching the overflow chunks below). A CPU_CACHED
    // pool would let the GPU read stale commands on real hardware.
    cmdMemStorage = InitMemoryPool(
        device,
        cmdMemPool,
        kCmdCommandMemorySize,
        nvn::MemoryPoolFlags::CPU_UNCACHED | nvn::MemoryPoolFlags::GPU_UNCACHED,
        0,
        false); // GPU uncached

    if (cmdMemStorage == nullptr) {
        forge_log_error("Failed to allocate memory for command buffer command memory pool");
        std::terminate();
    }

    controlMem = aligned_alloc(8, kCmdControlMemorySize);
    if (controlMem == nullptr) {
        forge_log_error("Failed to allocate memory for command buffer control memory");
        std::terminate();
    }

    cmdBuffer->SetMemoryCallback(MemoryCallback);
    cmdBuffer->SetMemoryCallbackData(this);
    ResetMemory();
}

CommandBufferAllocator::~CommandBufferAllocator()
{
    cmdMemPool.Finalize();
    free(cmdMemStorage);
    cmdMemStorage = nullptr;

    free(controlMem);
    controlMem = nullptr;

    for (auto& chunk : cmdMemChunks) {
        chunk->pool.Finalize();
        free(chunk->storage);
        chunk->storage = nullptr;
    }

    for (auto& chunk : controlMemChunks) {
        free(chunk->storage);
        chunk->storage = nullptr;
    }
}

void CommandBufferAllocator::ResetMemory()
{
    cmdMemChunkIndex = 0;
    controlMemChunkIndex = 0;

    cmdBuffer->AddCommandMemory(&cmdMemPool, 0, kCmdCommandMemorySize);
    cmdBuffer->AddControlMemory(controlMem, kCmdControlMemorySize);
}

void CommandBufferAllocator::AllocationCallback(nvn::CommandBuffer* cmdBuffer, nvn::CommandBufferMemoryEvent::Enum event, size_t minSize)
{
    if (event == nvn::CommandBufferMemoryEvent::OUT_OF_COMMAND_MEMORY) {
        if (minSize > kCommandMemChunkSize) {
            forge_log_error(
                "Command buffer requested command memory of size %zu which exceeds the chunk size of %zu."
                " Report this issue to the developer.",
                minSize, kCommandMemChunkSize);
            std::terminate();
        }

        if (cmdMemChunkIndex >= cmdMemChunks.size()) {
            auto chunk = std::make_unique<CommandMemoryChunk>();
            chunk->storage = InitMemoryPool(
                device,
                chunk->pool,
                kCommandMemChunkSize,
                nvn::MemoryPoolFlags::CPU_UNCACHED | nvn::MemoryPoolFlags::GPU_UNCACHED,
                0,
                false); // GPU Uncached

            if (chunk->storage == nullptr) {
                forge_log_error("Failed to allocate command buffer command memory");
                std::terminate();
            }

            cmdBuffer->AddCommandMemory(&chunk->pool, 0, kCommandMemChunkSize);
            cmdMemChunks.push_back(std::move(chunk));
        } else {
            auto& chunk = cmdMemChunks[cmdMemChunkIndex];
            cmdBuffer->AddCommandMemory(&chunk->pool, 0, kCommandMemChunkSize);
        }

        ++cmdMemChunkIndex;
    } else { // OUT_OF_CONTROL_MEMORY
        if (minSize > kControlMemChunkSize) {
            forge_log_error(
                "Command buffer requested control memory of size %zu which exceeds the chunk size of %zu."
                " Report this issue to the developer.",
                minSize, kControlMemChunkSize);
            std::terminate();
        }

        if (controlMemChunkIndex >= controlMemChunks.size()) {
            void* ptr = aligned_alloc(8, kControlMemChunkSize);
            if (ptr == nullptr) {
                forge_log_error("Failed to allocate command buffer control memory");
                std::terminate();
            }

            cmdBuffer->AddControlMemory(ptr, kControlMemChunkSize);
            controlMemChunks.push_back(std::make_unique<ControlMemoryChunk>(ptr));
        } else {
            auto& chunk = controlMemChunks[controlMemChunkIndex];
            cmdBuffer->AddControlMemory(chunk->storage, kControlMemChunkSize);
        }

        ++controlMemChunkIndex;
    }
}
