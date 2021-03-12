#include "core.hlsl"

#ifdef SV_PIXEL_SHADER

SV_TEXTURE(tex, t0);
SV_SAMPLER(sam, s0);

SV_CONSTANT_BUFFER(bloom_threshold_buffer, b0) {
       f32 threshold;
};

float4 main(float2 texcoord : FragTexcoord) : SV_Target0
{
	float3 color;

	float3 tex_color = tex.Sample(sam, texcoord).rgb;
	f32 brightness = dot(tex_color, float3(0.2126, 0.7152, 0.0722));

	if (brightness >= threshold) {

		return float4(tex_color, 1.f);
	}

	return = float4(0.f, 0.f, 0.f, 1.f); 
}

#endif