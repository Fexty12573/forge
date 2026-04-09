#include "forge/mem.h"

static Result getMap(MemoryInfo* info, u32 addr)
{
    u32 map;
    return svcQueryMemory(info, &map, addr);
}

static u32 getMapAddr(u32 addr)
{
    MemoryInfo map;
    getMap(&map, addr);
    return map.addr;
}

static u32 nextMap(u32 addr)
{
    MemoryInfo map;
    getMap(&map, addr);
    getMap(&map, map.addr + map.size);

    if (map.type != MemType_Unmapped)
        return map.addr;

    return nextMap(map.addr);
}

static u32 nextMapOfType(u32 addr, u32 type)
{
    MemoryInfo map;
    getMap(&map, addr);
    getMap(&map, map.addr + map.size);

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
