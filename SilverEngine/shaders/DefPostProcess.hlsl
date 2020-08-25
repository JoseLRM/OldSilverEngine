#include "core.hlsl"

// Vertex Shader
#ifdef SV_SHADER_TYPE_VERTEX

struct Input
{
    float2 position : Position;
};

struct Output
{
    float2 texCoord : TexCoord;
    float4 position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.position = float4(input.position, 0.f, 1.f);
    output.texCoord = (input.position / 2.f) + 0.5f;
    return output;
}

#endif

// Pixel Shader
#ifdef SV_SHADER_TYPE_PIXEL

struct Output
{
    float4 color : SV_Target;
};

SV_SAMPLER(sam, s0);
SV_IMAGE(tex, t0);

Output main(float2 TexCoord : TexCoord)
{
    Output output;
	output.color = tex.Sample(sam, TexCoord);
    return output;
}

#endif