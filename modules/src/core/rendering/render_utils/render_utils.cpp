#include "core.h"

#include "render_utils_internal.h"

namespace sv {

	static Time g_Timer;

	Result render_utils_initialize()
	{
		g_Timer = timer_now();

		svCheck(auximg_initialize());

		return Result_Success;
	}

	Result render_utils_close()
	{
		svCheck(auximg_close());

		return Result_Success;
	}

	void render_utils_update(f32 deltaTime)
	{
		Time now = timer_now();

		if (now - g_Timer >= 5.f) {
			auximg_update(now);

			g_Timer = now;
		}
	}

}