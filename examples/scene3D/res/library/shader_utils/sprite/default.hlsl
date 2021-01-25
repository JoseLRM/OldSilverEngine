#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float4 position : Position;
	float2 texCoord : TexCoord;
	float4 color : Color;
	float4 eColor : EmissionColor;
};

struct Output {
	float4 color : FragColor;
	float4 eColor : FragEmissionColor;
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;
};

Output main(Input input)
{
	Output output;
	output.color = input.color;
	output.eColor = input.eColor;
	output.texCoord = input.texCoord;
	output.position = input.position;
	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
	float4 color : FragColor;
	float4 eColor : FragEmissionColor;
	float2 texCoord : FragTexCoord;
};

struct Output {
	float4 color : SV_Target0;
	float4 eColor : SV_Target1;
};

SV_SAMPLER(sam, s0);
SV_TEXTURE(_Albedo, t0);

Output main(Input input)
{
	Output output;
	float4 texColor = _Albedo.Sample(sam, input.texCoord);
	if (texColor.a < 0.05f) discard;
	
	// Apply color
	output.color = input.color * texColor;
	output.eColor = input.eColor;

	return output;
}

#endif