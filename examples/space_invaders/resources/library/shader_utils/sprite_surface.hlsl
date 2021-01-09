
#name SpriteSurface
#shadertype PixelShader

struct Light {
	u32		type;
	float3	position;
	float3	color;
	float	range;
	float	intensity;
	float	smoothness;
};

struct SurfaceInput {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
};

struct SurfaceOutput {
	float4 color : SV_Target;
};

SV_SAMPLER(sam, s0);
SV_TEXTURE(_Albedo, t0);
SV_CONSTANT_BUFFER(LightBuffer, b0) {

	float3 ambientLight;
	u32 lightCount;
	Light lights[10];

};

// Lighting functions

float3 lightingPoint(Light light, float3 fragPosition) 
{
	float distance = length(light.position.xyz - fragPosition);
	
	float att = 1.f - smoothstep(light.range * light.smoothness, light.range, distance);
	return light.color * att * light.intensity;
}

float3 lightingResult(float3 fragPosition) 
{
	float3 lightColor = 0.f;
	foreach (i, lightCount) {

		Light light = lights[i];

		switch(light.type) {
		case SV_LIGHT_TYPE_POINT:
			lightColor += lightingPoint(light, fragPosition);
			break;
		}
	}

	return max(lightColor, ambientLight);
}

#userblock SpriteSurface

// Main
SurfaceOutput main(UserSurfaceInput input)
{
	return spriteSurface(input);
}
			