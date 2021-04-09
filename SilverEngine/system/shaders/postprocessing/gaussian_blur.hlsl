#include "core.hlsl"

#ifdef SV_PIXEL_SHADER

struct Input {
	float2 texcoord : FragTexcoord;
};

struct Output {
	float4 color : SV_Target0;
};

SV_GLOBAL u32 GAUSS_KERNEL = 33u;

SV_GLOBAL f32 WEIGHTS[GAUSS_KERNEL] = {
	0.004013,
	0.005554,
	0.007527,
	0.00999,
	0.012984,
	0.016524,
	0.020594,
	0.025133,
	0.030036,
	0.035151,
	0.040283,
	0.045207,
	0.049681,
	0.053463,
	0.056341,
	0.058141,
	0.058754,
	0.058141,
	0.056341,
	0.053463,
	0.049681,
	0.045207,
	0.040283,
	0.035151,
	0.030036,
	0.025133,
	0.020594,
	0.016524,
	0.012984,
	0.00999,
	0.007527,
	0.005554,
	0.004013
};

SV_TEXTURE(tex, t0);
SV_SAMPLER(sam, s0);

SV_CONSTANT_BUFFER(gauss_buffer, b0) {
	f32 intensity;
	u32 horizontal;
};

float4 main(float2 texcoord : FragTexcoord) : SV_Target0
{
	float4 color = float4(0.f, 0.f, 0.f, 1.f);

	float2 begin = texcoord;
	float2 adv;

	if (horizontal) {
		begin.x -= intensity * 0.5f;
		adv = float2(intensity / f32(GAUSS_KERNEL), 0.f);
	}
	else {
		begin.y -= intensity * 0.5f;
		adv = float2(0.f, intensity / f32(GAUSS_KERNEL));
	}

	foreach(i, GAUSS_KERNEL) {

		color.xyz += tex.Sample(sam, begin + adv * i).xyz * WEIGHTS[i];
	}

	return color;
}

#endif