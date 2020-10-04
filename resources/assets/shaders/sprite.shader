#name Sprite
#package default

#shader vs
#shader ps

#start
#include "core.hlsl"

// Vertex Shader
#ifdef SV_SHADER_TYPE_VERTEX 

struct Input
{
    float4 position : Position;
    float2 texCoord : TexCoord;
    float4 color : Color;
};

struct Output
{
    float4 color : FragColor;
    float2 texCoord : FragTexCoord;
    float4 position : SV_Position;
};


Output main(Input input)
{
    Output output;
	output.color = input.color;
	output.texCoord = input.texCoord;
	output.position = input.position;
    return output;
}

#endif

// Pixel Shader
#ifdef SV_SHADER_TYPE_PIXEL

struct Input
{
    float4 fragColor : FragColor;
    float2 fragTexCoord : FragTexCoord;
};

struct Output
{
    float4 color : SV_Target;
};

SV_SAMPLER(sam, s0);
SV_IMAGE(_Albedo, t0);

SV_DEFINE_MATERIAL(b0) {
    float radius;
};

Output main(Input input)
{
    Output output;
    float4 texColor = _Albedo.Sample(sam, input.fragTexCoord);
	if (texColor.a < 0.05f) discard;
    output.color = input.fragColor * texColor;

    float distance = length((input.fragTexCoord - 0.5f) * 2.f);
    if (distance > radius) output.color.xyz = float3(0.f, 0.6, 0.9f);
    else output.color.xyz = length(output.color.xyz) * 0.5f;
    

    return output;
}

#endif