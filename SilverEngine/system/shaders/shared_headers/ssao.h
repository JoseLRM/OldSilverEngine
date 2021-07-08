#ifndef SV_SHARED_SSAO
#define SV_SHARED_SSAO

struct GPU_SSAOData {
	u32 samples;
	f32 radius;
	f32 bias;
	f32 _padding0;
};


#endif
