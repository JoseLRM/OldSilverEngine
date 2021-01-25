#include "core.hlsl"

#ifdef SV_VERTEX_SHADER
struct Output {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;
};

Output main(float4 position : Position, float4 color : Color, float2 texCoord : TexCoord)
{
	Output o;
	o.position = position;
	o.color = color;
	o.texCoord = texCoord;
	return o;
}
#endif

#ifdef SV_PIXEL_SHADER
SV_SAMPLER(sam, s0);
SV_TEXTURE(tex, t0);

struct Output {
	float4 color : SV_Target0;
};

Output main(float4 color : FragColor, float2 texCoord : FragTexCoord)
{
	Output o;
	
	f32 a = tex.Sample(sam, texCoord).r;
	if (a < 0.08) discard;	

	o.color = float4(color.xyz * a, color.w);
	return o;
}
#endif