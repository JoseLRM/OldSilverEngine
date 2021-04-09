#ifndef _UTILS_RANDOM
#define _UTILS_RANDOM

// return value from 0u to (u32_max / 2u)
u32 math_random_u32(u32 seed)
{
	seed = (seed << 13) ^ seed;
	return ((seed * (seed * seed * 15731u * 789221u) + 1376312589u) & 0x7fffffffu);
	return seed * 0x4309F;
}

f32 math_random_f32(u32 seed)
{
	return (f32(math_random_u32(seed)) * (1.f / f32(u32_max / 2u)));
}
f32 math_random_f32(u32 seed, f32 _max)
{
	return math_random_f32(seed) * _max;
}
f32 math_random_f32(u32 seed, f32 _min, f32 _max)
{
	return _min + math_random_f32(seed) * (_max - _min);
}

u32 math_random_u32(u32 seed, u32 _max)
{
	return u32(math_random_f32(++seed, f32(_max)));
}
u32 math_random_u32(u32 seed, u32 _min, u32 _max)
{
	return u32(math_random_f32(++seed, f32(_min), f32(_max)));
}

#endif