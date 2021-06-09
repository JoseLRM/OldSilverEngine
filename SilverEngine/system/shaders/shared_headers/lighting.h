#ifndef SV_SHARED_LIGHTING
#define SV_SHARED_LIGHTING

struct GPU_LightData {
	v3_f32	  position;
	u32       type;
	v3_f32	  color;
	f32	      range;
	f32	      intensity;
	f32	      smoothness;
	u32       has_shadows;
	f32       padding0;
};

struct GPU_ShadowData {
	XMMATRIX light_matrix;
};

#ifndef __cplusplus

SV_CONSTANT_BUFFER(light_instances_buffer, b1) {
	GPU_LightData light;
};
SV_CONSTANT_BUFFER(shadow_data_buffer, b2) {
	GPU_ShadowData shadow_data;
};

SV_TEXTURE(shadow_map, t4);
SV_SAMPLER(sam, s0);

f32 compute_shadows(float3 position)
{
    float4 light_space = mul(float4(position, 1.f), shadow_data.light_matrix);

    f32 depth = shadow_map.Sample(sam, light_space.xy).r;

    return (light_space.z < (depth + 0.001f)) ? 1.f : 0.f;
}

float3 compute_light(float3 position, float3 normal, f32 specular_mul, f32 shininess, float3 specular_color)
{
    float3 acc = float3(0.f, 0.f, 0.f);
    
    switch (light.type) {

    case SV_LIGHT_TYPE_POINT:
    {
		float3 to_light = light.position - position;
		f32 distance = length(to_light);
		to_light = normalize(to_light);	

		// Diffuse
		f32 diffuse = max(dot(normal, to_light), 0.1f);
		acc += light.color * diffuse;

		// Specular
		f32 specular = pow(max(dot(normalize(-position), reflect(-to_light, normal)), 0.f), shininess) * specular_mul;
		acc += light.color * specular * specular_color;

		// attenuation TODO: Smoothness
		f32 att = 1.f - smoothstep(light.smoothness * light.range, light.range, distance);

		acc = acc * att * light.intensity;
	}
	break;

	case SV_LIGHT_TYPE_DIRECTION:
	{
	
		// Diffuse
		f32 diffuse = max(dot(normal, light.position), 0.f);
		acc += light.color * diffuse;

		// Specular
		float specular = pow(max(dot(normalize(-position), reflect(-light.position, normal)), 0.f), shininess) * specular_mul;
		acc += light.color * specular * specular_color;

		// Shadows
		if (light.has_shadows) {

			f32 shadow_mult = compute_shadows(position);
			acc *= shadow_mult;
		}

		acc = acc * light.intensity;
	}
	break;

	}

	return acc;
}

#endif

#endif
