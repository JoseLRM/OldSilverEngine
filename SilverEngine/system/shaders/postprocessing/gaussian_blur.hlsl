#include "core.hlsl"

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

#if TYPE == 0
SV_UAV_TEXTURE(dst, float4, u0);
SV_TEXTURE(src, t0);
#elif TYPE == 1
SV_UAV_TEXTURE(dst, float, u0);
SV_TEXTURE(src, t0);
#endif

SV_SAMPLER(sam, s0);

SV_CONSTANT_BUFFER(gauss_buffer, b0) {
	f32 intensity;
	u32 horizontal;
};

[numthreads(16,16,1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    float2 output_dimension;
    dst.GetDimensions(output_dimension.x, output_dimension.y);

    int2 screen_pos = dispatch_id.xy;
    
    float2 texcoord = (float2(screen_pos) + 0.5f) / output_dimension;

#if TYPE == 0
    float4 color = float4(0.f, 0.f, 0.f, 1.f);
#elif TYPE == 1
    float color = 0.f;
#endif

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
#if TYPE == 0
	color.xyz += src.SampleLevel(sam, begin + adv * i, 0).xyz * WEIGHTS[i];
#elif TYPE == 1
      	color += src.SampleLevel(sam, begin + adv * i, 0).r * WEIGHTS[i];
#endif
    }

    dst[screen_pos] = color;
}