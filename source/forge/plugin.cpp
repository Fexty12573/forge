#include "forge/plugin.h"
#include "forge/hook.h"
#include "forge/log.h"
#include "forge/mem.h"
#include "forge/proc.h"
#include "forge/types.h"
#include "nn/crypto.h"
#include "nn/ro.h"

#include <dirent.h>

#include <compare>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

template <size_t Alignment>
struct AlignedDeleter {
    void operator()(u8* ptr) const noexcept
    {
        ::operator delete[](ptr, std::align_val_t { Alignment });
    }
};

template <size_t Alignment>
using AlignedBuffer = std::unique_ptr<u8[], AlignedDeleter<Alignment>>;

struct ShaHash {
    u8 hash[0x20];

    auto operator<=>(const ShaHash& other) const = default;
};

struct PluginEvents {
    void (*on_load)();
    void (*on_update)(float dt);
};

struct Plugin {
    std::string path;
    AlignedBuffer<PAGE_SIZE> data;
    size_t data_size;
    nn::ro::Module module;
    AlignedBuffer<PAGE_SIZE> bss;
    size_t bss_size;
    ShaHash hash;
    PluginEvents events;

    template <typename T>
    T* getSymbol(const char* name) const
    {
        T* symbol;
        Result r = nn::ro::LookupModuleSymbol((uintptr_t*)&symbol, &this->module, name);
        if (R_FAILED(r)) {
            return nullptr;
        }

        return symbol;
    }
};

struct PluginLoader {
    std::vector<Plugin> plugins;
    nn::ro::RegistrationInfo registration_info;
    Hook update_hook;
    void (*original_update)(void*);
};

static PluginLoader s_pluginLoader { };

struct dirent64 {
    u64 d_ino;
    u64 d_off;
    u16 d_reclen;
    u8 d_type;
    char d_name[256];
};

struct dirent64* forge_readdir(DIR* dir)
{
    return (struct dirent64*)readdir(dir);
}

bool forge_isPluginFile(const char* filename)
{
    const char* ext = strrchr(filename, '.');
    return ext != NULL && strcmp(ext, ".nro") == 0;
}

constexpr size_t forge_alignUp(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

template <size_t Alignment>
AlignedBuffer<Alignment> forge_allocAligned(size_t size)
{
    return AlignedBuffer<Alignment>(static_cast<u8*>(::operator new[](size, std::align_val_t { Alignment })));
}

AlignedBuffer<PAGE_SIZE> forge_loadPluginData(const char* path, size_t* out_size)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    AlignedBuffer<PAGE_SIZE> data = forge_allocAligned<PAGE_SIZE>(size);
    fread(data.get(), 1, size, file);
    fclose(file);

    if (out_size) {
        *out_size = size;
    }

    return data;
}

void forge_plugin_onUpdateHook(void* main)
{
    s_pluginLoader.original_update(main);
    const auto delta_time = *(float*)((u8*)main + 0x68);

    for (const auto& plugin : s_pluginLoader.plugins) {
        if (plugin.events.on_update) {
            plugin.events.on_update(delta_time);
        }
    }
}

void forge_plugin_init(void)
{
    s_pluginLoader.update_hook = forge_hook_create(
        (void*)(g_mainTextAddr + 0x3DA8FC),
        (void*)forge_plugin_onUpdateHook,
        (void**)&s_pluginLoader.original_update);
}

void forge_plugin_loadPlugins(void)
{
    DIR* dir = opendir("app:/nativeNX/plugins");
    if (dir == NULL) {
        forge_log("Failed to open nativeNX/plugins");
        return;
    }

    Result r = nn::ro::Initialize();
    if (R_FAILED(r)) {
        forge_log("Failed to initialize ro: 0x%08X", r);
        closedir(dir);
        return;
    }

    std::set<ShaHash> plugin_hashes;

    struct dirent64* entry;
    while ((entry = forge_readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && forge_isPluginFile(entry->d_name)) {
            char path[512];
            snprintf(path, sizeof(path), "app:/nativeNX/plugins/%s", entry->d_name);

            forge_log("Found plugin file: %s", path);

            Plugin plugin;
            plugin.path = path;

            plugin.data = forge_loadPluginData(path, &plugin.data_size);
            if (plugin.data == nullptr) {
                forge_log("Failed to read plugin file %s", path);
                continue;
            }

            const auto nro_header = (nn::ro::NroHeader*)plugin.data.get();
            if (nro_header->magic != 0x304F524E) {
                forge_log("Invalid NRO file: %s", path);
                forge_log("Expected magic 0x304F524E, got 0x%08X", nro_header->magic);
                continue;
            }

            r = nn::ro::GetBufferSize(&plugin.bss_size, plugin.data.get());
            if (R_FAILED(r)) {
                forge_log("Failed to get BSS size for plugin %s: 0x%08X", path, r);
                continue;
            }

            nn::crypto::GenerateSha256Hash(&plugin.hash, sizeof(ShaHash), nro_header, nro_header->size);

            plugin_hashes.insert(plugin.hash);
            s_pluginLoader.plugins.emplace_back(std::move(plugin));
        }
    }

    closedir(dir);

    forge_log("Preparing to load %zu plugins", s_pluginLoader.plugins.size());

    const auto nrr_size = forge_alignUp(sizeof(nn::ro::NrrHeader) + s_pluginLoader.plugins.size() * sizeof(ShaHash), PAGE_SIZE);
    AlignedBuffer<PAGE_SIZE> nrr_data = forge_allocAligned<PAGE_SIZE>(nrr_size);

    auto nrr_header = (nn::ro::NrrHeader*)nrr_data.get();
    *nrr_header = {
        .magic = 0x3052524E,
        .program_id = { forge_proc_getProgramId() },
        .size = nrr_size,
        .type = 0,
        .hashes_offset = sizeof(nn::ro::NrrHeader),
        .num_hashes = (u32)plugin_hashes.size(),
    };

    ShaHash* hash_array = (ShaHash*)(nrr_data.get() + nrr_header->hashes_offset);

    for (const auto [i, hash] : plugin_hashes | std::views::enumerate) {
        hash_array[i] = hash;
    }

    r = nn::ro::RegisterModuleInfo(&s_pluginLoader.registration_info, nrr_data.get());
    if (R_FAILED(r)) {
        forge_log("Failed to register module info: 0x%08X", r);
        return;
    }

    for (auto& plugin : s_pluginLoader.plugins) {
        forge_log("Loading plugin %s", plugin.path.c_str());

        plugin.bss = forge_allocAligned<PAGE_SIZE>(plugin.bss_size);
        r = nn::ro::LoadModule(
            &plugin.module,
            plugin.data.get(),
            plugin.bss.get(),
            plugin.bss_size,
            nn::ro::BindFlag_Now);
        if (R_FAILED(r)) {
            forge_log("Failed to load plugin %s: 0x%08X", plugin.path.c_str(), r);
            continue;
        }
    }

    for (auto& plugin : s_pluginLoader.plugins) {
        forge_log("Resolving symbols for plugin %s", plugin.path.c_str());

        plugin.events.on_load = plugin.getSymbol<void()>("forge_onLoad");
        plugin.events.on_update = plugin.getSymbol<void(float)>("forge_onUpdate");

        if (!plugin.events.on_load) {
            forge_log("Plugin %s does not have a forge_onLoad function, skipping", plugin.path.c_str());
            continue;
        }

        plugin.events.on_load();
    }
}
