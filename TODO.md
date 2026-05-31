# TODO

* Check kernel version or if the kernel has the JIT patch for JIT

## ImGui Rendering

### Part 1

* [x] Hook `nvnBootstrapLoader` and return a wrapper for `nvnDeviceGetProcAddress`
* [ ] Return wrappers for
  * [x] `nvnDeviceInitialize`: Capture created device
  * [ ] `nvnQueueInitialize`: Capture created queue
  * [ ] `nvnWindowBuilderSetTextures`: Capture the swapchain textures
  * [ ] `nvnQueuePresentTexture`: Do own rendering

### Part 2

* [ ] Write custom imgui backend with NVN
  * [ ] Figure out a solution for compiling shaders
  * [ ] Figure out how the new texture system can be implemented

### Part 3

* [ ] Capture inputs (maybe `nn::hid` or game facilities) and forward to ImGui

### Part 4

* [ ] Implement `nvnQueuePresentTexture` hook with rendering
* [ ] Provide rendering callback for plugins
* [ ] Provide `ImGuiContext*` and allocator functions to plugins
