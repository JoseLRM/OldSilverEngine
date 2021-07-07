#pragma pack_matrix( row_major )

#ifdef SV_API_VULKAN

// VULKAN API
#ifdef SV_VERTEX_SHADER
#define SV_VK_RESOUCE_SET space0
#elif SV_PIXEL_SHADER
#define SV_VK_RESOUCE_SET space1
#elif SV_GEOMETRY_SHADER
#define SV_VK_RESOUCE_SET space2
#elif SV_COMPUTE_SHADER
#define SV_VK_RESOUCE_SET space0
#else
#error "SHADER NOT SUPPORTED"
#endif

#define SV_CONSTANT_BUFFER(name, binding) cbuffer name : register(binding, SV_VK_RESOUCE_SET)
#define SV_TEXTURE(name, binding) Texture2D name : register(binding, SV_VK_RESOUCE_SET)
#define SV_UAV_TEXTURE(name, temp, binding) RWTexture2D<temp> name : register(binding, SV_VK_RESOUCE_SET)
#define SV_CUBE_TEXTURE(name, binding) TextureCube name : register(binding, SV_VK_RESOUCE_SET)
#define SV_SAMPLER(name, binding) SamplerState name : register(binding, SV_VK_RESOUCE_SET)

#else
#error "API NOT SUPPORTED"
#endif



#include "shared_headers/core.h"

SV_CONSTANT_BUFFER(camera_buffer, SV_SLOT_CAMERA) { GPU_CameraData camera; };

struct Environment {
       float3 ambient_light;
};

// Helper functions

float3 compute_fragment_position(f32 depth, float2 texcoord, matrix ipm)
{
	float4 pos = float4(texcoord.x * 2.f - 1.f, texcoord.y * 2.f - 1.f, depth, 1.f);
	pos = mul(pos, ipm);
	return pos.xyz / pos.w;
}