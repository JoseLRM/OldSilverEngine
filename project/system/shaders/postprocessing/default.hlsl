#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Output {
	float2 texcoord : FragTexcoord;
	float4 position : SV_Position;
};

Output main(u32 id : SV_VertexID)
{
	Output output;
	output.texcoord = float2((id << 1) & 2, id & 2);
	output.position = float4(output.texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
	/*
	switch (id)
	{
	case 0:
		output.position = float4(-1.f, -1.f, 0.f, 1.f);
		break;
	case 1:
		output.position = float4(1.f, -1.f, 0.f, 1.f);
		break;
	case 2:
		output.position = float4(-1.f, 1.f, 0.f, 1.f);
		break;
	case 3:
		output.position = float4(1.f, 1.f, 0.f, 1.f);
		break;
	}

	output.texcoord = (output.position.xy + 1.f) / 2.f;
	*/
	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

SV_TEXTURE(tex, t0);
SV_SAMPLER(sam, s0);

float4 main(float2 texcoord : FragTexcoord) : SV_Target0
{
	return tex.Sample(sam, texcoord);
}

#endif