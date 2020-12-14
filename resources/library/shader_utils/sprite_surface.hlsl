
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

#userblock SpriteSurface

// Main
SurfaceOutput main(UserSurfaceInput input)
{
	return spriteSurface(input);
}
			