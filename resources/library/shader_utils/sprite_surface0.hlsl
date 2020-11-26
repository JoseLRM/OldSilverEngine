
struct SurfaceInput {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
};

struct SurfaceOutput {
	float4 color : SV_Target;
};

SV_SAMPLER(sam, s0);
SV_TEXTURE(_Albedo, t0);
			