#include "core.hlsl"

SV_UAV_TEXTURE(dst, float4, u0);
SV_TEXTURE(src, t0);
SV_SAMPLER(sam, s0);

[numthreads(16, 16, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    float2 size;
    dst.GetDimensions(size.x, size.y);
    int2 screen_pos = dispatch_id.xy;
    
    float2 texcoord = (float2(screen_pos) + 0.5f) / size;
    
    dst[screen_pos].xyz += src.SampleLevel(sam, texcoord, 0).xyz;
}