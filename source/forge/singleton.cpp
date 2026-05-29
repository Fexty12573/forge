#include "forge/singleton.h"
#include "Mt/MtCRC.h"
#include "Mt/MtDti.h"
#include "Mt/MtObject.h"
#include "forge/hook.h"
#include "forge/log.h"
#include "forge/mem.h"

#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <vector>

static std::vector<void*> s_singletons { };
static std::unordered_map<u32, void*> s_singletonMap { };

static Hook s_cSystemCtorHook;
static Hook s_cSystemDtorHook;
static void* (*s_originalSystemCtor)(void*);
static void* (*s_originalSystemDtor)(void*);

void* cSystemCtorHook(void* sys)
{
    s_singletons.emplace_back(sys);
    return s_originalSystemCtor(sys);
}

void* cSystemDtorHook(void* sys)
{
    std::erase(s_singletons, sys);
    std::erase_if(s_singletonMap, [sys](const auto& it) { return it.second == sys; });

    return s_originalSystemDtor(sys);
}

extern "C" void forge_singleton_init(void)
{
    const auto ctor = (void*)(g_mainTextAddr + 0x887810);
    const auto dtor = (void*)(g_mainTextAddr + 0x88784C);

    s_cSystemCtorHook = forge_hook_create(ctor, (void*)cSystemCtorHook, (void**)&s_originalSystemCtor);
    s_cSystemDtorHook = forge_hook_create(dtor, (void*)cSystemDtorHook, (void**)&s_originalSystemDtor);
}

extern "C" void forge_singleton_resolve(void)
{
    forge_log_info("Resolving Singletons...");

    for (const auto instance : s_singletons) {
        const auto obj = (MtObject*)instance;
        const auto type = obj->getDti();

        s_singletonMap[type->id] = instance;

        if (type->name != nullptr) {
            forge_log_info("Resolved Singleton: %s @ 0x%X", type->name, (uintptr_t)instance);
        } else {
            forge_log_info("Resolved Singleton: 0x%08X @ 0x%X", type->id, (uintptr_t)instance);
        }
    }

    s_singletons.clear();
}

extern "C" void* forge_singleton_getInstanceByName(const char* name)
{
    return forge_singleton_getInstanceById(MtDti::makeId(name));
}

extern "C" void* forge_singleton_getInstanceById(u32 id)
{
    const auto iter = s_singletonMap.find(id);
    if (iter != s_singletonMap.end()) {
        return iter->second;
    }

    return nullptr;
}

u32 forge_singleton_getAllInstances(void** out, u32 max)
{
    if (out == nullptr) {
        return s_singletonMap.size();
    }

    u32 written = 0;
    for (const auto [_, instance] : s_singletonMap) {
        if (written >= max) {
            break;
        }

        out[written++] = instance;
    }

    return written;
}
