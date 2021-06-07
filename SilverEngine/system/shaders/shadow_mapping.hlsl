#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float3 position : Position;
	float3 normal : Normal;
	float4 tangent : Tangent;
	float2 texcoord : Texcoord;
};

SV_CONSTANT_BUFFER(shadow_mapping_buffer, b0) {
	matrix tm;
};

float4 main(Input input) : SV_Position
{
    return mul(float4(input.position, 1.f), tm);  
}

#endif