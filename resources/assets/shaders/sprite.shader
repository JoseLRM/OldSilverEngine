#name default/Sprite

#type Sprite

// Vertex Shader
#VS_begin

#include "core.hlsl"

SV_DEFINE_CAMERA(b0);

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
	output.position = mul(camera.vpm, input.position);
    return output;
}

#VS_end

// Pixel Shader
#PS_begin

#include "core.hlsl"

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
SV_TEXTURE(_Albedo, t0);

SV_DEFINE_MATERIAL(b0)
{
    float opacidad;
    float2 pene;
}

SV_TEXTURE(Juan, t1);

Output main(Input input)
{
    Output output;

    float4 texColor = _Albedo.Sample(sam, input.fragTexCoord) * Juan.Sample(sam, input.fragTexCoord);
    output.color = input.fragColor * texColor;
	if (output.color.a < 0.05f) discard;

    output.color.a = opacidad;
    output.color.x *= pene.x;
    output.color.y *= pene.y;

    return output;
}

#PS_end