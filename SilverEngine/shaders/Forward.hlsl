#ifdef SV_SHADER_TYPE_PIXEL
#define SV_ENABLE_LIGHTING
#endif

#include "core.hlsl"

// VERTEX SHADER
#ifdef SV_SHADER_TYPE_VERTEX

struct Input
{
    float3 position : Position;
    float3 normal : Normal;
    float2 texCoord : TexCoord;
    matrix modelViewMatrix : ModelViewMatrix;
};

struct Output
{
    float3 normal : FragNormal;
    float3 fragPos : FragPosition;
    float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(Camera, b0)
{
    matrix projectionMatrix;
}

Output main(Input input)
{
    Output output;
    output.normal = input.normal;
    float4 fragPos = mul(input.modelViewMatrix, float4(input.position, 1.f));
    output.fragPos = fragPos.xyz;
    output.position = mul(fragPos, projectionMatrix);
    return output;
}

#endif

// PIXEL SHADER
#ifdef SV_SHADER_TYPE_PIXEL

struct Input
{
    float3 normal : FragNormal;
    float3 position : FragPosition;
};

struct Output
{
    float4 color : SV_Target;
};

SV_CONSTANT_BUFFER(Material, b0)
{
    float4 diffuseColor;
}

SV_LIGHT_BUFFER(b1);

Output main(Input input)
{
    float3 lighting = float3(0.f, 0.f, 0.f);
    for(ui32 i = 0; i < SV_LIGHT_COUNT; ++i)
	    lighting += lighting_diffuse(input.position, input.normal, lights[i]);

    Output output;
    output.color = diffuseColor * float4(lighting, 1.f);
    return output;

}

#endif