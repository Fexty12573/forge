#pragma once

#include <span>

#include <imgui.h>
#include <nvn/nvn_Cpp.h>

IMGUI_IMPL_API void ImGui_ImplNVN_Init(nvn::Device* device, nvn::Queue* queue, std::span<nvn::Texture*> swapChainTextures);
IMGUI_IMPL_API void ImGui_ImplNVN_Shutdown();
IMGUI_IMPL_API void ImGui_ImplNVN_NewFrame();
IMGUI_IMPL_API void ImGui_ImplNVN_SetSwapChainTextures(std::span<nvn::Texture*> swapChainTextures);
IMGUI_IMPL_API void ImGui_ImplNVN_RenderDrawData(nvn::Queue* queue, ImDrawData* drawData, int textureIndex);

// Registers a caller-owned texture in the backend's descriptor pools and returns a handle usable with ImGui::Image.
// The texture (and view) must stay alive until it is unregistered and all frames that drew it have completed.
IMGUI_IMPL_API ImTextureID ImGui_ImplNVN_RegisterTexture(const nvn::Texture* texture, const nvn::TextureView* view = nullptr);
IMGUI_IMPL_API void ImGui_ImplNVN_UnregisterTexture(ImTextureID textureId);
