#include "core.hlsl"
#include "shared_headers/ssao.h"

SV_TEXTURE(normal_map, t0);
SV_TEXTURE(depth_map, t1);
SV_UAV_TEXTURE(output, float, u0);

SV_CONSTANT_BUFFER(ssao_buffer, b0) {
GPU_SSAOData data;
};

[numthreads(16,16,1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
	float2 output_dimension;
	output.GetDimensions(output_dimension.x, output_dimension.y);
	
	int2 screen_pos = dispatch_id.xy;	

	f32 depth = depth_map.Load(int3(screen_pos.x, screen_pos.y, 0)).r;
	if (depth >= 0.999999f) {
	   output[screen_pos] = 1.f;
	   return;
	}

	float3 normal = normal_map.Load(int3(screen_pos.x, screen_pos.y, 0)).xyz;
	float3 position = compute_fragment_position(depth, (float2(screen_pos) + 0.5f) / output_dimension, camera.ipm);

	f32 occlusion = 0.f;

	u32 seed = (screen_pos.x << 16) | screen_pos.y;

	u32 samples = data.samples;

	// TODO: Adjust
	if (depth >= 0.9) {
	   samples *= 4;
	}
	else if (depth >= 0.9999) {
	   samples *= 2;
	}
	else if (depth < 0.999999) {
	   samples /= 2;
	}

	foreach(i, samples) {

		   // Compute random hemisphere position in view space

		   f32 scale = f32(i) / (f32)data.samples;
		   scale = lerp(0.1f, 1.f, scale * scale);

		   u32 i0 = (seed * (i + 1) * 9853454);
		   u32 i1 = (seed * (i + 1) * 4748245);
		   u32 i2 = (seed * (i + 1) * 8734542);

		   float3 sample;
		   sample.x = (f32(i0 % 10000) / 10000.f) * 2.f - 1.f;
		   sample.y = (f32(i1 % 10000) / 10000.f) * 2.f - 1.f;
		   sample.z = (f32(i2 % 10000) / 10000.f) * 2.f - 1.f;

		   f32 len = length(sample);
		   sample /= len;

		   if (dot(normal, sample) < 0.0f) {

		      sample = -sample;
		   };

		   sample = (sample * len * data.radius) + position;

		   // Project sample in clip space

		   float4 sample_projection = mul(float4(sample, 1.f), camera.pm);
		   sample_projection.xyz /= sample_projection.w;
		   float2 offset = sample_projection.xy  * 0.5f + 0.5f;

		   // Compute sample position

		   int2 sample_screen = int2(offset * output_dimension);

		   if (sample_screen.x >= 0 && sample_screen.x <= output_dimension.x &&
		      sample_screen.y >= 0 && sample_screen.y <= output_dimension.y &&
		      (sample_screen.x != screen_pos.x || sample_screen.y != screen_pos.y)) {

		      f32 sample_depth = depth_map.Load(int3(sample_screen.x, sample_screen.y, 0)).r;

		      float3 sampled_point = compute_fragment_position(sample_depth, offset, camera.ipm);

		      f32 range = smoothstep(0.f, 1.f, data.radius / abs(sample.z - sampled_point.z));

		      occlusion += ((sample.z - data.bias >= sampled_point.z) ? 1.5f : 0.f) * range;  
		   }
	}

	output[screen_pos] = 1.f - (occlusion / f32(samples));
}