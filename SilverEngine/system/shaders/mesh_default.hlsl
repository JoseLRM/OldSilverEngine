#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float3 position : Position;
	float3 normal : Normal;
	float4 tangent : Tangent;
	float2 texcoord : Texcoord;
};

struct Output {
	float3 frag_position : FragPosition;
	float3 normal : FragNormal;
	float3 tangent : FragTangent;
	float3 bitangent : FragBitangent;
	float2 texcoord : FragTexcoord;
	float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(instance_buffer, b0) {
	matrix mvm;
	matrix imvm;
};

SV_CAMERA_BUFFER(b1);

Output main(Input input)
{
	Output output;

	float4 pos = mul(float4(input.position, 1.f), mvm);
	output.frag_position = pos.xyz;
	output.position = mul(pos, camera.pm);

	float3 tangent = input.tangent.xyz;
	float3 bitangent = cross(input.normal, tangent) * input.tangent.w;

	output.normal = mul((float3x3)imvm, input.normal);
	output.tangent = mul((float3x3)imvm, tangent);
	output.bitangent = mul((float3x3)imvm, bitangent);

	output.texcoord = input.texcoord;

	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
	float3 position : FragPosition;
	float3 normal : FragNormal;
	float3 tangent : FragTangent;
	float3 bitangent : FragBitangent;
	float2 texcoord : FragTexcoord;
};

struct Output {
	float4 color : SV_Target0;
	float4 normal : SV_Target1;
	float4 emission : SV_Target2;
};

struct Material {
	float3	diffuse_color;
	u32	flags;
	float3	specular_color;
	f32	shininess;
	float3	emissive_color;
};

#define MAT_FLAG_NORMAL_MAPPING SV_BIT(0u)
#define MAT_FLAG_SPECULAR_MAPPING SV_BIT(1u)

SV_CONSTANT_BUFFER(material_buffer, b0) {
	Material material;
};

#define LIGHT_TYPE_POINT 0u
#define LIGHT_TYPE_DIRECTION 1u

struct Light {
	float3 position;
	u32    type;
	float3 color;
	f32    range;
	f32    intensity;
	f32    smoothness;
};

#define LIGHT_COUNT 1u

SV_CONSTANT_BUFFER(light_instances_buffer, b1) {
	Light lights[LIGHT_COUNT];
};

SV_CONSTANT_BUFFER(environment_buffer, b2) {
	Environment environment;
};

SV_TEXTURE(diffuse_map, t0);
SV_TEXTURE(normal_map, t1);
SV_TEXTURE(specular_map, t2);
SV_TEXTURE(emissive_map, t3);

SV_SAMPLER(sam, s0);

Output main(Input input)
{
	Output output;

	// DIFFUSE
	float4 diffuse_color = diffuse_map.Sample(sam, input.texcoord);
	if (diffuse_color.a < 0.01f) discard;

	diffuse_color.rgb *= material.diffuse_color;

	// NORMAL
	float3 normal;

	if (material.flags & MAT_FLAG_NORMAL_MAPPING) {

		float3x3 TBN = transpose(float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal)));

		normal = normal_map.Sample(sam, input.texcoord).xyz * 2.f - 1.f;
		normal = mul(TBN, normal);
	}
	else
		normal = normalize(input.normal);

	output.normal = float4(normal, 0.f);

	// SPECULAR
	float specular_mul;
	if (material.flags & MAT_FLAG_SPECULAR_MAPPING) {
		specular_mul = specular_map.Sample(sam, input.texcoord).r;
	}
	else specular_mul = 1.f;

	// Compute lighting
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

			float3 acc = float3(0.f, 0.f, 0.f);

			// Diffuse
			f32 diffuse = max(dot(normal, to_light), 0.1f);
			acc += light.color * diffuse;

			// Specular
			f32 specular = pow(max(dot(normalize(-input.position), reflect(-to_light, normal)), 0.f), material.shininess) * specular_mul;
			acc += light.color * specular * material.specular_color;

			// attenuation TODO: Smoothness
			f32 att = 1.f - smoothstep(light.smoothness * light.range, light.range, distance);

			light_accumulation += acc * att * light.intensity;
		}
		break;

		case LIGHT_TYPE_DIRECTION:
		{
		        float3 acc = float3(0.f, 0.f, 0.f);
			
			// Diffuse
			f32 diffuse = max(dot(normal, light.position), 0.f);
			acc += light.color * diffuse;

			// Specular
			float specular = pow(max(dot(normalize(-input.position), reflect(-light.position, normal)), 0.f), material.shininess) * specular_mul;
			acc += light.color * specular * material.specular_color;

			light_accumulation += acc * light.intensity;
		}
		break;

		}
	}

	// Ambient lighting
	light_accumulation = max(environment.ambient_light, light_accumulation);

	output.color = float4(diffuse_color.rgb * light_accumulation, 1.f);
	
	// Emissive	
	output.emission.rgb = material.emissive_color;
	// TEMP
	output.emission.a = 1.f;

	return output;
}

#endif