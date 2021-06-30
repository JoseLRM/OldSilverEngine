#ifndef SV_SHARED_CORE
#define SV_SHARED_CORE

#ifdef __cplusplus

#include "defines.h"

using namespace sv;

#else

// Types

typedef unsigned int    u32;
typedef int             i32;
typedef float           f32;
typedef double          f64;
typedef matrix          XMMATRIX;
typedef float2          v2_f32;
typedef float3          v3_f32;
typedef float4          v4_f32;

// Macros

#define foreach(_it, _end) for (u32 _it = 0u; _it < _end; ++_it)
#define SV_BIT(x) (1ULL << x) 
#define SV_GLOBAL static const

SV_GLOBAL u32 u32_max = 0xFFFFFFFF;

#endif

// Slots

#ifdef __cplusplus

#define SV_SLOT_CAMERA 31

#else

#define SV_SLOT_CAMERA b31

#endif

#define SV_LIGHT_TYPE_POINT 0u
#define SV_LIGHT_TYPE_DIRECTION 1u

// Structs

struct GPU_CameraData {
    XMMATRIX vm;
    XMMATRIX pm;
    XMMATRIX vpm;
    XMMATRIX ivm;
    XMMATRIX ipm;
    XMMATRIX ivpm;
    f32      near;
    f32      far;
	f32      width;
	f32      height;
    v3_f32   position;
    f32      _padding0;
    v4_f32   rotation;
	v2_f32   screen_size;
	v2_f32   _padding1;
};

#endif
