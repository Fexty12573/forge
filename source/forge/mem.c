#include "forge/mem.h"
#include "forge/proc.h"
#include <string.h>

Result forge_mem_write(u32 addr, const void* data, size_t size)
{
    const u8* src = data;
    while (size) {
        MemoryInfo info;
        Result rc = forge_mem_queryMemory(&info, addr);
        if (R_FAILED(rc)) {
            return rc;
        }
        const size_t remaining = info.addr + info.size - addr;
        const size_t chunk = size < remaining ? size : remaining;
        if (info.perm & Perm_W) {
            memcpy((void*)addr, src, chunk);
        } else {
            // Map a writable alias; leave the original executable mapping in place.
            const uintptr_t page = addr & ~(uintptr_t)0xFFF;
            const size_t offset = addr - page;
            const size_t map_size = (offset + chunk + 0xFFF) & ~(size_t)0xFFF;
            virtmemLock();
            void* alias = virtmemFindAslr(map_size, 0x1000);
            if (alias == NULL) {
                virtmemUnlock();
                return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
            }
            const Handle process = forge_proc_getHandle();
            rc = svcMapProcessMemory(alias, process, page, map_size);
            virtmemUnlock();
            if (R_FAILED(rc)) {
                return rc;
            }
            memcpy((u8*)alias + offset, src, chunk);
            rc = svcFlushProcessDataCache(process, addr, chunk);
            const Result unmap_rc = svcUnmapProcessMemory(alias, process, page, map_size);
            if (R_FAILED(rc)) {
                return rc;
            }
            if (R_FAILED(unmap_rc)) {
                return unmap_rc;
            }
        }
        addr += chunk;
        src += chunk;
        size -= chunk;
    }
    return 0;
}

extern void forge_sdkGenerateRandomBytes(void*, size_t) __asm__("_ZN2nn2os19GenerateRandomBytesEPvj");

// Avoid libnx random.c, which also assumes libnx thread-local storage.
uintptr_t __libnx_virtmem_rng(void)
{
    uintptr_t value;
    forge_sdkGenerateRandomBytes(&value, sizeof(value));
    return value;
}

static u32 getMapAddr(u32 addr)
{
    MemoryInfo map;
    forge_mem_queryMemory(&map, addr);
    return map.addr;
}

static u32 nextMap(u32 addr)
{
    MemoryInfo map;
    forge_mem_queryMemory(&map, addr);
    forge_mem_queryMemory(&map, map.addr + map.size);

    if (map.type != MemType_Unmapped)
        return map.addr;

    return nextMap(map.addr);
}

static u32 nextMapOfType(u32 addr, u32 type)
{
    MemoryInfo map;
    forge_mem_queryMemory(&map, addr);
    forge_mem_queryMemory(&map, map.addr + map.size);

    if (map.type == type)
        return map.addr;

    return nextMapOfType(map.addr, type);
}

void nninitStartup();

u32 g_mainTextAddr;
u32 g_mainRodataAddr;
u32 g_mainDataAddr;
u32 g_mainBssAddr;
u32 g_mainHeapAddr;

void forge_mem_init()
{
    // nninitStartup can be reasonably assumed to be exported by main
    g_mainTextAddr = getMapAddr((u32)nninitStartup);
    g_mainRodataAddr = nextMap(g_mainTextAddr);
    g_mainDataAddr = nextMap(g_mainRodataAddr);
    g_mainBssAddr = nextMap(g_mainDataAddr);
    g_mainHeapAddr = nextMapOfType(g_mainBssAddr, MemType_Heap);
}

Result forge_mem_queryMemory(MemoryInfo* out_info, u32 addr)
{
    u32 map;
    return svcQueryMemory(out_info, &map, addr);
}

u32 forge_mem_getMainTextAddr(void)
{
    return g_mainTextAddr;
}

u32 forge_mem_getMainRodataAddr(void)
{
    return g_mainRodataAddr;
}

u32 forge_mem_getMainDataAddr(void)
{
    return g_mainDataAddr;
}

u32 forge_mem_getMainBssAddr(void)
{
    return g_mainBssAddr;
}

u32 forge_mem_getMainHeapAddr(void)
{
    return g_mainHeapAddr;
}
