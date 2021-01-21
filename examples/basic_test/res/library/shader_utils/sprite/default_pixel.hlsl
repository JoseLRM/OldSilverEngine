#include "core.hlsl"

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