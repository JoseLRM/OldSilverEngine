#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
    float3 position : Position;
    float3 normal : Normal;
};

struct Output {
    float3 normal : FragNormal;
    float3 frag_position : FragPosition;
    float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(instance_buffer, b0) {
    matrix tm;
};

SV_CONSTANT_BUFFER(camera_buffer, b1) {
    Camera camera;
};

Output main(Input input) 
{
    Output output;

    matrix mvm = mul(tm, camera.vm);

    float4 pos = mul(float4(input.position, 1.f), mvm);
    output.frag_position = pos.xyz;
    output.position = mul(pos, camera.pm);
    output.normal = mul((float3x3)mvm, input.normal);

    return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
    float3 normal : FragNormal;
    float3 position : FragPosition;
};

struct Output {
    float4 color : SV_Target0;
};

Output main(Input input) 
{
    Output output;

    output.color = float4(abs(input.normal), 1.f);

    return output;
}

#endif