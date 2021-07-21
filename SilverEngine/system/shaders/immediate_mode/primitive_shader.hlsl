#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Output {
       float2 texcoord : FragTexcoord;
       float4 color : FragColor;
       float4 position : SV_Position;
};

struct Vertex {
       float4 position;
       float2 texcoord;
       u32 color;
       u32 padding;
};

SV_CONSTANT_BUFFER(immediate_buffer, b0)
{
	Vertex vertices[4];
};

Output main(u32 vertex_id : SV_VertexID)
{
	Output output;

	Vertex v = vertices[vertex_id];

	output.texcoord = v.texcoord;
	output.position = v.position;

	const f32 mult = 1.f / 255.f;
	output.color.a = f32(v.color >> 24) * mult;
	output.color.b = f32((v.color >> 16) & 0xFF) * mult;
	output.color.g = f32((v.color >> 8) & 0xFF) * mult;
	output.color.r = f32(v.color & 0xFF) * mult;

	return output;
}


#endif

#ifdef SV_PIXEL_SHADER

struct Input {
       float2 texcoord : FragTexcoord;
       float4 color : FragColor;
};

SV_TEXTURE(tex, t0);
SV_SAMPLER(sam, s0);

float4 main(Input input) : SV_Target0
{
	float4 color = tex.Sample(sam, input.texcoord) * input.color;
	if (color.a < 0.0001f)
	   discard;
	return color;
}


#endif