#include "core.hlsl"

#ifdef SV_PIXEL_SHADER

struct Output {
	float4 color : SV_Target0;
};

#define LIGHT_TYPE_POINT 0u
#define LIGHT_TYPE_DIRECTION 1u

struct Light {
	float3 position;
	u32 type;
	float3 color;
	f32	range;
	f32 intensity;
	f32 smoothness;
};

#define LIGHT_COUNT 1u

SV_CONSTANT_BUFFER(light_instances_buffer, b0) {
	Light lights[LIGHT_COUNT];
};

SV_CONSTANT_BUFFER(environment_buffer, b1) {
	Environment environment;
};

SV_CAMERA_BUFFER(b2);

SV_TEXTURE(diffuse_map, t0);
SV_TEXTURE(normal_map, t1);
SV_TEXTURE(depth_map, t2);
//SV_TEXTURE(specular_map, t2);
//SV_TEXTURE(emissive_map, t3);

SV_SAMPLER(sam, s0);

Output main(float2 texcoord : FragTexcoord)
{
	Output output;

	float depth = depth_map.Sample(sam, texcoord).r;

	if (depth > 0.999999f)
	   discard;

	float3 diffuse_color = diffuse_map.Sample(sam, texcoord).rgb;
	float3 normal = normal_map.Sample(sam, texcoord).rgb;
	// TODO float4 specular_sample = specular_map.Sample(sam, texcoord);
	//float3 specular_color = specular_sample.rgb;
	//f32 shininess = specular_sample.a;

	float3 specular_color = float3(0.1f, 0.1f, 0.1f);
	f32 shininess = 0.5f;
	f32 specular_mul = 0.1f;

	float4 pos = float4(texcoord.x * 2.f - 1.f, texcoord.y * 2.f - 1.f, depth, 1.f);
	pos = mul(pos, camera.ipm);
	float3 position = pos.xyz / pos.w;
	
	// Compute lighting
	float3 light_accumulation = float3(0.f, 0.f, 0.f);

	[unroll]
	foreach(i, LIGHT_COUNT) {

		Light light = lights[i];

		switch (light.type) {

		case LIGHT_TYPE_POINT:
		{
			float3 to_light = light.position - position;
			f32 distance = length(to_light);
			to_light = normalize(to_light);

			// Diffuse
			f32 diffuse = max(dot(normal, to_light), 0.1f);

			// Specular
			f32 specular = pow(max(dot(normalize(-position), reflect(-to_light, normal)), 0.f), shininess) * specular_mul;

			// attenuation TODO: Smoothness
			f32 att = 1.f - smoothstep(light.smoothness * light.range, light.range, distance);

			//light_accumulation += light.color * specular * specular_color * light.intensity * att;
			light_accumulation += light.color * light.intensity * att * diffuse;
		}
		break;

		case LIGHT_TYPE_DIRECTION:
		{
			// Diffuse
			f32 diffuse = max(dot(normal, light.position), 0.f);

			// Specular
			float specular = pow(max(dot(normalize(-position), reflect(-light.position, normal)), 0.f), shininess) * specular_mul;

			light_accumulation += light.color * (diffuse_color.rgb + (specular * specular_color)) * light.intensity;
		}
		break;

		}
	}

	// Ambient lighting
	light_accumulation = max(environment.ambient_light, light_accumulation);

	output.color = float4(diffuse_color * light_accumulation, 1.f);

	return output;
}

#endif