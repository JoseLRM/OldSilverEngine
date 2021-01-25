#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
    float4 position : Position;
    float2 texCoord : TexCoord;
    float stroke : Stroke;
    float4 color : Color;
};

struct Output {
	float4 color : PxColor;
	float2 texCoord : PxTexCoord;
	float2 stroke : PxStroke;
	float4 position : SV_Position;
};

Output main(Input input)
{
	Output output;
	output.position = input.position;
	output.color = input.color;
	float2 halfSize = abs(input.texCoord);
	output.texCoord = input.texCoord;
	output.stroke = halfSize - (input.stroke * min(halfSize.x, halfSize.y));
	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
    float4 color : PxColor;
    float2 texCoord : PxTexCoord;
    float2 stroke : PxStroke;
};

struct Output {
	float4 color : SV_Target;
};

Output main(Input input)
{
	Output output;
	if (abs(input.texCoord.x) < input.stroke.x && abs(input.texCoord.y) < input.stroke.y) discard;
	output.color = input.color;
	return output;
};

#endif