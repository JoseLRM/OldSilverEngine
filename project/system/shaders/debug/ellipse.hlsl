#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
	float4 position : Position;

	float2 texCoord : TexCoord;
	float stroke : Stroke;
	float4 color : Color;
};
struct Output {
	float2 texCoord : PxTexCoord;
	float4 color : PxColor;
	float stroke : PxStroke;
	float4 position : SV_Position;
};
Output main(Input input)
{
	Output output;
	output.position = input.position;
	output.color = input.color;
	output.texCoord = input.texCoord;
	float2 halfSize = abs(input.texCoord);
	float s = min(halfSize.x, halfSize.y);
	output.stroke = s - (input.stroke * s);
	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
	float2 texCoord : PxTexCoord;
	float4 color : PxColor;
	float stroke : PxStroke;
};
struct Output {
	float4 color : SV_Target;
};
Output main(Input input)
{
	float distance = length(input.texCoord);
	if (distance > 0.5f || distance < input.stroke) discard;
	Output output;
	output.color = input.color;
	return output;
}

#endif