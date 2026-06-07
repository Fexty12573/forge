#pragma once

#include <nvn/nvn_Cpp.h>

#include <memory>
#include <vector>

struct CommandMemoryChunk {
    nvn::MemoryPool pool;
    void* storage;
};

struct ControlMemoryChunk {
    void* storage;
};

class CommandBufferAllocator {
public:
    CommandBufferAllocator(nvn::Device* device, nvn::CommandBuffer* cmdBuffer);
    ~CommandBufferAllocator();

    void ResetMemory();
    void AllocationCallback(nvn::CommandBuffer* cmdBuffer, nvn::CommandBufferMemoryEvent::Enum event, size_t minSize);

private:
    nvn::Device* device;
    nvn::CommandBuffer* cmdBuffer;
    nvn::MemoryPool cmdMemPool;
    void* cmdMemStorage;
    void* controlMem;

    std::vector<std::unique_ptr<CommandMemoryChunk>> cmdMemChunks;
    std::vector<std::unique_ptr<ControlMemoryChunk>> controlMemChunks;
    size_t cmdMemChunkIndex;
    size_t controlMemChunkIndex;
};

void* InitMemoryPool(nvn::Device* device, nvn::MemoryPool& pool, size_t size, nvn::MemoryPoolFlags extraFlags = 0, size_t minAlignment = 0, bool gpuCached = true);

template <size_t Align>
size_t AlignUp(size_t value)
{
    return (value + Align - 1) & ~(Align - 1);
}
