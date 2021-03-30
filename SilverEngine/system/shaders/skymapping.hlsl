#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float3 position : Position;
};

struct Output {

	float4 position : SV_Position;
	float3 texcoord : FragTexcoord;

};

SV_CONSTANT_BUFFER(cubemap_buffer, b0) {
	matrix vpm;
};

Output main(Input input)
{
	Output output;
	
	output.texcoord = input.position;
	output.position = mul(float4(input.position, 1.f), vpm);

	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
	float3 texcoord : FragTexcoord;
};

struct Output {
	float4 color : SV_Target;
};

SV_CUBE_TEXTURE(skymap, t0);
SV_SAMPLER(sam, s0);

Output main(Input input) 
{
	Output output;
	output.color = skymap.Sample(sam, input.texcoord);
	return output;
}

#endif