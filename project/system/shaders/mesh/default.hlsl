#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
    float3 position : Position;
    float3 normal : Normal;
};

struct Output {
    float3 normal : FragNormal;
    float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(cbufferpro, b0) {
    matrix m;
};

Output main(Input input) 
{
    Output output;

    output.position = mul(m, float4(input.position, 1.f));
    output.normal = mul((float3x3)m, input.normal);

    return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
    float3 normal : FragNormal;
};

struct Output {
    float4 color : SV_Target0;
};

Output main(Input input) 
{
    Output output;

    output.color = float4(input.normal, 1.f);

    return output;
}

#endif