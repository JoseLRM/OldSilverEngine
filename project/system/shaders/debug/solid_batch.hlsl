#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
    float4 position : Position;
    float4 color : Color;
};

struct Output {
	float4 color : FragColor;
	float4 position : SV_Position;
};

Output main(Input input)
{
	Output output;
	output.position = input.position;
	output.color = input.color;
	return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
    float4 color : FragColor;
};

struct Output {
	float4 color : SV_Target0;
};

Output main(Input input)
{
	Output output;
	output.color = input.color;
	return output;
};

#endif