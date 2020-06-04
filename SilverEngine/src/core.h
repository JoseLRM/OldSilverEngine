#pragma once

#include "..//config.h"

// std includes
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <stdint.h>

// types
typedef uint8_t		ui8;
typedef uint16_t	ui16;
typedef uint32_t	ui32;
typedef uint64_t	ui64;

typedef int8_t		i8;
typedef int16_t		i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef wchar_t		wchar;

// macros
#ifdef SV_DEBUG
#define SV_ASSERT(x) do{ if((x) == false) exit(1); }while(false)
#else
#define SV_ASSERT(x)
#endif

#define SV_FATAL_ERROR(x)
#define SV_BIT(x) 1 << x

// logging
namespace SV {
	void Log(const char* s, ...);
	void LogI(const char* s, ...);
	void LogW(const char* s, ...);
	void LogE(const char* s, ...);
}