#pragma pack_matrix( row_major )

#ifdef SV_API_VULKAN

// VULKAN API
#ifdef SV_VERTEX_SHADER
#define SV_VK_RESOUCE_SET space0
#elif SV_PIXEL_SHADER
#define SV_VK_RESOUCE_SET space1
#elif SV_GEOMETRY_SHADER
#define SV_VK_RESOUCE_SET space2
#else
#error "SHADER NOT SUPPORTED"
#endif

#define SV_CONSTANT_BUFFER(name, binding) cbuffer name : register(binding, SV_VK_RESOUCE_SET)
#define SV_TEXTURE(name, binding) Texture2D name : register(binding, SV_VK_RESOUCE_SET)
#define SV_CUBE_TEXTURE(name, binding) TextureCube name : register(binding, SV_VK_RESOUCE_SET)
#define SV_SAMPLER(name, binding) SamplerState name : register(binding, SV_VK_RESOUCE_SET)

#else
#error "API NOT SUPPORTED"
#endif

// Types

typedef unsigned int    u32;
typedef int             i32;
typedef float           f32;
typedef double          f64;

// Macros

#define foreach(_it, _end) for (u32 _it = 0u; _it < _end; ++_it)
#define SV_BIT(x) (1ULL << x) 
#define SV_GLOBAL static const

SV_GLOBAL u32 u32_max = 0xFFFFFFFF;

// Structs

struct Camera {
    matrix vm;
    matrix pm;
    matrix vpm;
    matrix ivm;
    matrix ipm;
    matrix ivpm;
    float2 screen_size;
    float  near;
    float  far;
    float3 position;
    float _padding0;
    float4 direction;
};

#define SV_CAMERA_BUFFER(slot) SV_CONSTANT_BUFFER(camera_buffer, slot) { Camera camera; };

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