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

// Structs

struct Camera {
    matrix vm;
    matrix pm;
    matrix vpm;
    float3 position;
    float4 direction;
};

struct Environment {
       float3 ambient_light;
};     