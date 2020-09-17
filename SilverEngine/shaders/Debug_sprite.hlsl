#include "core.hlsl"

// VERTEX SHADER
#ifdef SV_SHADER_TYPE_VERTEX

struct Input {
    float4 position : Position;
    float2 texCoord : TexCoord;
    float stroke : Stroke;
    float4 color : Color;
};

struct Output {
    float4 color : PxColor;
    float2 texCoord : PxTexCoord;
    float4 position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.position = input.position;
    output.color = input.color;
    output.texCoord = input.texCoord;
    return output;
}

#endif

// PIXEL SHADER
#ifdef SV_SHADER_TYPE_PIXEL

struct Input {
    float4 color : PxColor;
    float2 texCoord : PxTexCoord;
};

struct Output {
    float4 color : SV_Target;
};

SV_IMAGE(tex, t0);
SV_SAMPLER(sam, s0);

Output main(Input input)
{
    Output output;
    output.color = input.color * tex.Sample(sam, input.texCoord);
    return output;
}

#endif
