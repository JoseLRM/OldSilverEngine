#pragma once

#include "core.h"

#ifdef SV_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#include "platform/windows_impl.h"
#endif

#ifdef SV_PLATFORM_LINUX
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include "..//external/vk_mem_alloc.h"

#ifdef SV_DEBUG
#define vkAssert(x) if((x) != VK_SUCCESS) SV_THROW("VulkanError", #x)
#else
#define vkAssert(x) x
#endif

#define vkCheck(x) if((x) != VK_SUCCESS) return false
#define vkExt(x) do{ VkResult res = (x); if(res != VK_SUCCESS) return res; }while(0)

#undef CreateSemaphore

#include "../external/sprv/spirv_reflect.hpp"