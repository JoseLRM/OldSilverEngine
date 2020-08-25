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
#define SV_IMAGE(name, binding) Texture2D name : register(binding, SV_VK_RESOUCE_SET)
#define SV_SAMPLER(name, binding) SamplerState name : register(binding, SV_VK_RESOUCE_SET)

#else
#error "API NOT SUPPORTED"
#endif