#pragma once

#include "..//config.h"

// std includes
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

// types
typedef uint8_t		ui8;
typedef uint16_t	ui16;
typedef uint32_t	ui32;
typedef uint64_t	ui64;

// macros
#define SL_ASSERT(x) do{ if((x) == false) exit(1); }while(false)
#define BIT(x) 1 << x