#include "core.hlsl"
#include "utils/random.hlsl"

#ifdef SV_PIXEL_SHADER

SV_TEXTURE(normal_map, t0);
SV_TEXTURE(depth_map, t1);

SV_GLOBAL u32 NUM_SAMPLES = 64u;
SV_GLOBAL f32 RADIUS = 0.5f;

float main(float2 texcoord : FragTexcoord) : SV_Target0
{
	int2 screen_pos = (int2)(camera.screen_size * texcoord);


	f32 depth = depth_map.Load(int3(screen_pos.x, screen_pos.y, 0)).r;
	if (depth >= 0.99999f)
	   discard;

	float3 normal = normal_map.Load(int3(screen_pos.x, screen_pos.y, 0)).xyz;
	float3 position = compute_fragment_position(depth, texcoord, camera.ipm);

	f32 occlusion = 0.f;

	[unroll]
	foreach(i, NUM_SAMPLES) {

		   // Compute random hemisphere position in view space

		   f32 scale = f32(i) / 64.f;
		   scale = lerp(0.1f, 1.f, scale * scale);

		   float3 sample;

		   sample.x = (f32(i * 0xFF32A323 % 5484u) / f32(5484u) * 2.f - 1.f) * scale;
		   sample.y = (f32(i * 0x43F32B32 % 5484u) / f32(5484u) * 2.f - 1.f) * scale;
		   sample.z = (f32(i * 0x3254ADFF % 5484u) / f32(5484u) * 2.f - 1.f) * scale;

		   if (dot(normal, sample) < 0.f) {

		      sample = -sample;
		   }

		   sample = sample * RADIUS + position;

		   // Project sample in clip space

		   float4 sample_projection = mul(float4(sample, 1.f), camera.pm);
		   float2 offset = sample_projection.xy / sample_projection.w;

		   // Compute sample position

		   int2 sample_screen = int2((offset.xy * 0.5f + 0.5f) * camera.screen_size);
		   f32 sample_depth = depth_map.Load(int3(sample_screen.x, sample_screen.y, 0)).r;

		   sample_projection = mul(float4(offset.x, offset.y, sample_depth, 1.f), camera.ipm);

		   float3 sample_position = float3(sample_projection.xyz / sample_projection.w);

		   f32 range = smoothstep(0.f, 1.f, RADIUS / abs(position.z - sample_position.z));

		   occlusion += ((dot(sample_position - position, normal) > 0.1f) ? range : 0.f);
	}

	return 1.f - (occlusion / f32(NUM_SAMPLES));
}

#endif