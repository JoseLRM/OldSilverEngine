#name Sprite
#package default

#shader vs
#shader ps

#start
#include "core.hlsl"

// Vertex Shader
#ifdef SV_SHADER_TYPE_VERTEX

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
	output.position = mul(input.position, camera.viewProjectionMatrix);
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
SV_TEXTURE(_Albedo, t0);

SV_TEXTURE(Test, t1);

SV_DEFINE_MATERIAL(b0) {
    float4 diffuse;
    float alpha;
    float blend;
};

Output main(Input input)
{
    Output output;
    float4 texColor0 = _Albedo.Sample(sam, input.fragTexCoord);
    float4 texColor1 = Test.Sample(sam, input.fragTexCoord);
    output.color = input.fragColor * texColor0 * blend + texColor1 * (1.f - blend);
    output.color *= diffuse;
    output.color.w *= alpha;

	if (output.color.a < 0.05f) discard;
    return output;
}

#endif