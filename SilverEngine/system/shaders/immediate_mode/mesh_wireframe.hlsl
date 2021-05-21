#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float3 position : Position;
	float3 normal : Normal;
	float3 tangent : Tangent;
	float3 bitangent : Bitangent;
	float2 texcoord : Texcoord;
};

SV_CONSTANT_BUFFER(mesh_data, b0) {
	matrix tm;
	float4 color;
};

struct Output {
	float2 texcoord : FragTexcoord;
       	float4 color : FragColor;
	float4 position : SV_Position;
};

Output main(Input input) {

	Output output;

	output.position = mul(float4(input.position, 1.f), tm);
	output.color = color;
	output.texcoord = float2(0.f, 0.f);

	return output;
}

#endif