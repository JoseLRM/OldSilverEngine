
#name SpriteSurface
#shadertype PixelShader

struct SurfaceInput {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
};

struct SurfaceOutput {
	float4 color : SV_Target;
};

SV_SAMPLER(sam, s0);
SV_TEXTURE(_Albedo, t0);

#userblock SpriteSurface
// User Callbacks
// struct UserSurfaceInput : SurfaceInput {}
// SurfaceOutput spriteSurface(UserSurfaceInput input);

// Main
SurfaceOutput main(UserSurfaceInput input)
{
	return spriteSurface(input);
}
			