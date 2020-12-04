#include "core.h"

#include "material_system_internal.h"

#include "utils/io.h"

namespace sv {

	InstanceAllocator<ShaderLibraryType_internal, 10u>	g_ShaderLibrariesTypes;
	InstanceAllocator<ShaderLibrary_internal, 50u>		g_ShaderLibraries;
	InstanceAllocator<Material_internal, 100u>			g_Materials;

	Result matsys_initialize()
	{
		// Register the assets
		svCheck(matsys_asset_register());

		// Write shader utils
		{
			graphics_shader_include_write("core",
				R"(
#ifdef SV_API_VULKAN

// VULKAN API
#ifdef SV_SHADER_TYPE_VERTEX
#define SV_VK_RESOUCE_SET space0
#elif SV_SHADER_TYPE_PIXEL
#define SV_VK_RESOUCE_SET space1
#elif SV_SHADER_TYPE_GEOMETRY
#define SV_VK_RESOUCE_SET space2
#else
#error "SHADER NOT SUPPORTED"
#endif

#define SV_CONSTANT_BUFFER(name, binding) cbuffer name : register(binding, SV_VK_RESOUCE_SET)
#define SV_TEXTURE(name, binding) Texture2D name : register(binding, SV_VK_RESOUCE_SET)
#define SV_SAMPLER(name, binding) SamplerState name : register(binding, SV_VK_RESOUCE_SET)

#else
#error "API NOT SUPPORTED"
#endif

// Types

typedef unsigned int    ui32;
typedef int             i32;

struct Camera {
    matrix vm;
    matrix pm;
    matrix vpm;
    matrix ivm;
    matrix ipm;
    matrix ivpm;
    float3 position;
    float4 direction;
};

#define SV_DEFINE_MATERIAL(binding) SV_CONSTANT_BUFFER(__Material__, binding)
#define SV_DEFINE_CAMERA(binding) SV_CONSTANT_BUFFER(__Camera__, binding) { Camera camera; };
				)");
		}

		return Result_Success;
	}

	void matsys_update()
	{
		Material_internal::update();

		// Unbind camera buffers from shader libraries
		{
			for (auto& pool : g_ShaderLibraries) {
				for (ShaderLibrary_internal& lib : pool) {
					lib.cameraBuffer = nullptr;
				}
			}
		}
	}

	Result matsys_close()
	{
		ui32 count;

		// Warning and destroy unfreed objects
		{
			count = g_Materials.unfreed_count();
			if (count) SV_LOG_WARNING("There are %u unfreed Materials", count);

			count = g_ShaderLibraries.unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed Shader Libraries", count);

				for (auto& pool : g_ShaderLibraries) {
					for (ShaderLibrary_internal& lib : pool) {
						SV_LOG_ERROR("TODO");
					}
				}
			}

		}

		// Clear allocators
		{
			g_ShaderLibrariesTypes.clear();
			g_ShaderLibraries.clear();
			g_Materials.clear();
		}

		g_MaterialsToUpdate.clear();

		return Result_Success;
	}

}