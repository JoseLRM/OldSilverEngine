#include "core.hlsl"

SV_UAV_TEXTURE(dst, float4, u0);
SV_TEXTURE(src, t0);

[numthreads(16, 16, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    int2 screen_pos = dispatch_id.xy;
    dst[screen_pos] += src[screen_pos];
}