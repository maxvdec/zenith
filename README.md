# Zenith  
Zenith is a tool designed to make developing games or anything that requires a Graphics API the fastest way possible.
Zenith packs renderers like Vulkan, OpenGL or Metal into a single library, so you can use the same code for all platforms.

## Features
It gives a nice-looking C++ safe API, which is easy to use and understand.

## Roadmap
* [x] Setting up the project
* [ ] Creating a simple abstraction layer
* [ ] **Vulkan Renderer**
  * [ ] Setting up the surface
  * [ ] Create the render process
  * [ ] Add support for shaders (in native SPIR-V format)
  * [ ] Add support for textures
  * [ ] Add support for framebuffers
  * [ ] Add support for multisampling
  * [ ] Better configuration of the renderer
  * [ ] Creating depth buffers
  * [ ] Creating stencil buffers
  * [ ] Add mipmapping support
  * [ ] Texture compression
  * [ ] Instancing
  * [ ] Compute pipeline and shaders
  * [ ] Geometry shaders
  * [ ] Tessellation shaders
  * [ ] Bindless resources
  * [ ] Pipeline caching
  * [ ] Multiple rendering targets
  * [ ] Occlusion queries
  * [ ] Timestamp / GPU Time queries
  * [ ] Swapchain support
  * [ ] Dynamic state (viewport, scissor, etc.)
  * [ ] Synchronization primitives (fences, semaphores, events)
  * [ ] Multi-threaded rendering
  * [ ] *Ray tracing as extension*
  * [ ] *Debug layer as extension*