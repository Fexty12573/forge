#pragma once

#include <imgui.h>
#include <nvn/nvn_Cpp.h>

IMGUI_IMPL_API void ImGui_ImplNVN_Init(nvn::Device* device, nvn::Queue* queue);
IMGUI_IMPL_API void ImGui_ImplNVN_Shutdown();
