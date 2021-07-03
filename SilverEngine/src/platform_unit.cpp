#if SV_PLATFORM_WIN
#include "platform/win64.cpp"
#elif SV_PLATFORM_LINUX

#endif

#include "platform/os.cpp"
#include "platform/memory.cpp"
#include "platform/graphics.cpp"
#include "platform/graphics_shader.cpp"
#include "platform/vulkan/graphics_vulkan.cpp"
#include "platform/vulkan/graphics_vulkan_pipeline.cpp"
