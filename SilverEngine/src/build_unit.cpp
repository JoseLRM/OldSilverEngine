
#if SV_PLATFORM_WIN
#include "platform/win64_engine.cpp"
#elif SV_PLATFORM_LINUX

#endif

#include "engine.cpp"
#include "core.cpp"
#include "utils.cpp"
#include "console.cpp"
#include "renderer/renderer.cpp"
#include "renderer/debug_renderer.cpp"
#include "renderer/font.cpp"
#include "scene.cpp"
#include "editor.cpp"
#include "editor_gui.cpp"
#include "mesh.cpp"
#include "gui.cpp"
#include "asset_system.cpp"
#include "platform/graphics.cpp"
#include "platform/graphics_shader.cpp"
#include "platform/vulkan/graphics_vulkan.cpp"
#include "platform/vulkan/graphics_vulkan_pipeline.cpp"
