#include "forge/mem.h"

Result memGetMap(MemoryInfo* info, u32 addr)
{
    u32 map;
    return svcQueryMemory(info, &map, addr);
}

u32 memGetMapAddr(u32 addr)
{
    MemoryInfo map;
    memGetMap(&map, addr);
    return map.addr;
}

u32 memNextMap(u32 addr)
{
    MemoryInfo map;
    memGetMap(&map, addr);
    memGetMap(&map, map.addr + map.size);

    if (map.type != MemType_Unmapped)
        return map.addr;

    return memNextMap(map.addr);
}

u32 memNextMapOfType(u32 addr, u32 type)
{
    MemoryInfo map;
    memGetMap(&map, addr);
    memGetMap(&map, map.addr + map.size);

    if (map.type == type)
        return map.addr;

    return memNextMapOfType(map.addr, type);
}

void nninitStartup();

u32 g_mainTextAddr;
u32 g_mainRodataAddr;
u32 g_mainDataAddr;
u32 g_mainBssAddr;
u32 g_mainHeapAddr;

void memSetup()
{
    // nninitStartup can be reasonably assumed to be exported by main
    g_mainTextAddr = memGetMapAddr((u32)nninitStartup);
    g_mainRodataAddr = memNextMap(g_mainTextAddr);
    g_mainDataAddr = memNextMap(g_mainRodataAddr);
    g_mainBssAddr = memNextMap(g_mainDataAddr);
    g_mainHeapAddr = memNextMapOfType(g_mainBssAddr, MemType_Heap);
}
