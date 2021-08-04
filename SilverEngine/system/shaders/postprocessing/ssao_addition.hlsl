#include "core.hlsl"

SV_UAV_TEXTURE(offscreen, float4, u0);
SV_TEXTURE(ssao_map, t0);

[numthreads(16, 16, 1)]
void main(uint3 coord : SV_DispatchThreadID)
{
    float ssao = ssao_map.Load(coord).r;
    offscreen[coord.xy].rgb *= ssao;
}