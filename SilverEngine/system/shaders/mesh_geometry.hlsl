#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float3 position : Position;
	float3 normal : Normal;
	float3 tangent : Tangent;
	float3 bitangent : Bitangent;
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

	output.normal = mul((float3x3)imvm, input.normal);
	output.tangent = mul((float3x3)imvm, input.tangent);
	output.bitangent = mul((float3x3)imvm, input.bitangent);

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
	float4 diffuse : SV_Target0;
	float4 normal : SV_Target1;
};

struct Material {
	float3	diffuse_color;
	u32		flags;
	float3	specular_color;
	f32		shininess;
	float3	emissive_color;
};

#define MAT_FLAG_NORMAL_MAPPING SV_BIT(0u)
#define MAT_FLAG_SPECULAR_MAPPING SV_BIT(1u)

SV_CONSTANT_BUFFER(material_buffer, b0) {
	Material material;
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
	float4 diffuse = diffuse_map.Sample(sam, input.texcoord);
	if (diffuse.a < 0.01f) discard;
	output.diffuse = diffuse;

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

	// TODO: SPECULAR
	//float specular_mul;
	//if (material.flags & MAT_FLAG_SPECULAR_MAPPING) {
		//specular_mul = specular_map.Sample(sam, input.texcoord).r;
	//}
	//else specular_mul = 1.f;
	
	
	

	// TODO: Emissive

	return output;
}

#endif