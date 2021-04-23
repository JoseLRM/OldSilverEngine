// PLATFORM LAYER

#if SV_PLATFORM_WIN
#include "platform/win64.cpp"
#elif SV_PLATFORM_LINUX

#endif

// PLATFORM

#include "platform/os.cpp"
#include "platform/graphics.cpp"
#include "platform/graphics_shader.cpp"
#include "platform/vulkan/graphics_vulkan.cpp"
#include "platform/vulkan/graphics_vulkan_pipeline.cpp"

// UTILS

#include "utils/allocators.cpp"
#include "utils/math.cpp"
#include "utils/serialize.cpp"

// CORE

#include "core/engine.cpp"
#include "core/renderer/renderer.cpp"
#include "core/renderer/debug_renderer.cpp"
#include "core/renderer/font.cpp"
#include "core/scene.cpp"
#include "core/mesh.cpp"
#include "core/gui.cpp"
#include "core/asset_system.cpp"
#include "core/event_system.cpp"

// DEBUG

#include "debug/console.cpp"
#include "debug/editor.cpp"
#include "debug/editor_gui.cpp"

// USER

#include "user.cpp"
