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

// Types

typedef unsigned int    ui32;

typedef int             i32;

// LIGHTING

#ifdef SV_ENABLE_LIGHTING

// Types

#define SV_LIGTH_TYPE_POINT 1
#define SV_LIGTH_TYPE_SPOT 2
#define SV_LIGTH_TYPE_DIRECTIONAL 3

struct Light
{
    ui32    lightType;
	float3	position;
	float3	direction;
	float	intensity;
	float	range;
	float3	color;
    float   smoothness;
    float3  padding;
};

#define SV_LIGHT_BUFFER(binding) SV_CONSTANT_BUFFER(LightBuffer, binding) { Light lights[SV_LIGHT_COUNT]; }

float3 lighting_compute_diffuse_color(float3 toLight, float3 normal, float3 color)
{
    return color * max(dot(toLight, normal), 0.f);
}

float3 lighting_compute_specular_color(float3 toLight, float3 normal, float3 color)
{
    return float3(0.f, 0.f, 0.f);
}

float lighting_compute_attenuation(float range, float smoothness, float distance)
{
    return 1.f - smoothstep(range * smoothness, range, distance);
}

float3 lighting_diffuse(float3 position, float3 normal, Light light)
{
    switch(light.lightType) {

        case SV_LIGTH_TYPE_POINT:
        {
            float3 toLight = light.position - position;
            float distance = length(toLight);
            toLight = normalize(toLight);

            float3 diffuseColor = lighting_compute_diffuse_color(toLight, normal, light.color);
            float3 specularColor = lighting_compute_specular_color(toLight, normal, light.color);

            float attenuation = lighting_compute_attenuation(light.range, light.smoothness, distance);

            return (diffuseColor + specularColor) * attenuation * light.intensity;
        }
        default:
            return float3(0.f, 0.f, 0.f);
    }
}

#endif