#pragma once

#include <span>

#include <imgui.h>
#include <nvn/nvn_Cpp.h>

IMGUI_IMPL_API void ImGui_ImplNVN_Init(nvn::Device* device, nvn::Queue* queue, std::span<nvn::Texture*> swapChainTextures);
IMGUI_IMPL_API void ImGui_ImplNVN_Shutdown();
IMGUI_IMPL_API void ImGui_ImplNVN_NewFrame();
IMGUI_IMPL_API void ImGui_ImplNVN_SetSwapChainTextures(std::span<nvn::Texture*> swapChainTextures);
IMGUI_IMPL_API void ImGui_ImplNVN_RenderDrawData(nvn::Queue* queue, ImDrawData* drawData, int textureIndex);
