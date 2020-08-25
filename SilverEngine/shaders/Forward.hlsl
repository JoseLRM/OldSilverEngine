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
    float3 fragNormal : FragNormal;
    float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(Camera, b0)
{
    matrix pm;
}

Output main(Input input)
{
    Output output;
    output.fragNormal = input.normal;
    output.position = mul(mul(input.modelViewMatrix, float4(input.position, 1.f)), pm);
    return output;
}

#endif

// PIXEL SHADER
#ifdef SV_SHADER_TYPE_PIXEL

struct Output
{
    float4 color : SV_Target;
};

SV_CONSTANT_BUFFER(Material, b0)
{
    float4 diffuseColor;
}

Output main(float3 FragNormal : FragNormal)
{
	
    Output output;
    output.color = float4(FragNormal, 1.f) * diffuseColor;
    return output;

}

#endif