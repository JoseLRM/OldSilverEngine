#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
    float3 position : Position;
    float3 normal : Normal;
	float2 texcoord : Texcoord;
};

struct Output {
    float3 frag_position : FragPosition;
    float3 normal : FragNormal;
	float2 texcoord : FragTexcoord;
    float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(instance_buffer, b0) {
    matrix mvm;
	matrix imvm;
};

SV_CONSTANT_BUFFER(camera_buffer, b1) {
    Camera camera;
};

Output main(Input input) 
{
    Output output;

    float4 pos = mul(mvm, float4(input.position, 1.f));
    output.frag_position = pos.xyz;
    output.position = mul(camera.pm, pos);
    output.normal = mul((float3x3)imvm, input.normal);
	output.texcoord = input.texcoord;

    return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
    float3 position : FragPosition;
    float3 normal : FragNormal;
	float2 texcoord : FragTexcoord;
};

struct Output {
    float4 color : SV_Target0;
};

struct Material {
	float3	diffuse_color;
	f32		shininess;
	float3	specular_color;
};

#define LIGHT_TYPE_POINT 1u
#define LIGHT_TYPE_DIRECTION 1u

struct Light {
	float3 position;
	u32 type;
	f32	range;
	f32 intensity;
	f32 smoothness;
	f32 _padding0;
	float3 color;
	f32 _padding1;
};

#define LIGHT_COUNT 10u

SV_CONSTANT_BUFFER(material_buffer, b0) {
	Material material;
};

SV_CONSTANT_BUFFER(light_instances_buffer, b1) {
	Light lights[LIGHT_COUNT];
};

SV_TEXTURE(diffuse_map, t0);

SV_SAMPLER(sam, s0);

Output main(Input input) 
{
    Output output;

	float3 normal = normalize(input.normal);
	float3 light_accumulation = float3(0.f, 0.f, 0.f);

	[unroll]
	foreach(i, LIGHT_COUNT) {

		Light light = lights[i];

		switch (light.type) {

		case LIGHT_TYPE_POINT:
		{
			float3 to_light = light.position - input.position;
			f32 distance = length(to_light);
			to_light = normalize(to_light);

			// Diffuse
			f32 diffuse = max(dot(normal, to_light), 0.1f);

			// Specular
			float specular = pow(max(dot(normalize(-input.position), reflect(-to_light, normal)), 0.f), material.shininess);

			// TODO attenuation and intensity

			light_accumulation += light.color * ((diffuse * material.diffuse_color) + (specular * material.specular_color));
		}
		break;

		}
	}

    float4 diffuse = diffuse_map.Sample(sam, input.texcoord);
    
    output.color = diffuse * float4(light_accumulation, 1.f);

    return output;
}

#endif