#include "core.hlsl"

SV_UAV_TEXTURE(dst, float4, u0);
SV_TEXTURE(src, t0);
SV_SAMPLER(sam, s0);

SV_CONSTANT_BUFFER(bloom_threshold_buffer, b0) {
       f32 threshold;
       f32 strength;
};

[numthreads(16, 16, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    float2 size;
    dst.GetDimensions(size.x, size.y);
    int2 screen_pos = dispatch_id.xy;
    
    float2 texcoord = (float2(screen_pos) + 0.5f) / size;

    float3 color;

    float3 tex_color = src.SampleLevel(sam, texcoord, 0).rgb;
    f32 brightness = dot(tex_color, float3(0.2126, 0.7152, 0.0722));

    if (brightness >= threshold) {

	color = tex_color * strength;
    }
    else color = float3(0.f, 0.f, 0.f);

    dst[screen_pos] = float4(color, 1.f);
}