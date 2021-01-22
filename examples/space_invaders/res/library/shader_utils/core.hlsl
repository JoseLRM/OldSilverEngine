
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

typedef unsigned int    u32;
typedef int             i32;
typedef float           f32;
typedef double          f64;

// Macros

#define foreach(_it, _end) for (u32 _it = 0u; _it < _end; ++_it)

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

#define SV_LIGHT_TYPE_POINT 0

#define SV_DEFINE_MATERIAL(binding) SV_CONSTANT_BUFFER(__Material__, binding)
#define SV_DEFINE_CAMERA(binding) SV_CONSTANT_BUFFER(__Camera__, binding) { Camera camera; };
				