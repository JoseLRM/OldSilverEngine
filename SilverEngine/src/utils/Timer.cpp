#include "core.h"

namespace SV {
	namespace Timer {

		std::chrono::steady_clock::time_point g_InitialTime;

		std::chrono::steady_clock::time_point NowFromChrono()
		{
			return std::chrono::steady_clock::now();
		}

		void _internal::Initialize()
		{
			g_InitialTime = NowFromChrono();
		}

		Time Now()
		{
			return Time(std::chrono::duration<float>(NowFromChrono() - g_InitialTime).count());
		}
	}
}