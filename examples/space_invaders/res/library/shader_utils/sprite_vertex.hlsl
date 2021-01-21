#include "core.hlsl"

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