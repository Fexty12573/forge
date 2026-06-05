# TODO

* Check kernel version or if the kernel has the JIT patch for JIT

## ImGui Rendering

### Part 1

* [x] Hook `nvnBootstrapLoader` and return a wrapper for `nvnDeviceGetProcAddress`
* [x] Return wrappers for
  * [x] `nvnDeviceInitialize`: Capture created device
  * [x] `nvnQueueInitialize`: Capture created queue
  * [x] `nvnWindowBuilderSetTextures`: Capture the swapchain textures
  * [x] `nvnQueuePresentTexture`: Do own rendering

### Part 2

* [x] Write custom imgui backend with NVN
  * [x] Figure out a solution for compiling shaders
  * [x] Figure out how the new texture system can be implemented

### Part 3

* [x] Capture inputs (maybe `nn::hid` or game facilities) and forward to ImGui

### Part 4

* [x] Implement `nvnQueuePresentTexture` hook with rendering
* [x] Provide rendering callback for plugins
* [ ] Provide `ImGuiContext*` and allocator functions to plugins
