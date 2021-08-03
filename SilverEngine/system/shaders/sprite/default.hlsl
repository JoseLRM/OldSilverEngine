#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float4 position : Position;
	float2 texCoord : TexCoord;
	float4 color : Color;
	float4 emissive_color : EmissiveColor;
};

struct Output {
       	float4 color : FragColor;
	float4 emissive_color : FragEmissiveColor;
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;
};

Output main(Input input)
{
	Output output;
	output.color = input.color;
	output.emissive_color = input.emissive_color;
	output.texCoord = input.texCoord;
	output.position = input.position;
	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
	float4 color : FragColor;
	float4 emissive_color : FragEmissiveColor;
	float2 texCoord : FragTexCoord;
};

struct Output {
	float4 color : SV_Target0;
	float4 normal : SV_Target1;
	float4 emissive_color : SV_Target2;
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
	output.emissive_color = input.emissive_color;

	return output;
}

#endif