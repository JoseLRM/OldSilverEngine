#pragma once

#include "core.h"

#include "utils/Version.h"
#include "platform/window.h"

namespace sv {

	struct ApplicationCallbacks {
		Result(*initialize)();
		void(*update)(float);
		void(*render)();
		Result(*close)();
	};

	struct InitializationDesc {

		ApplicationCallbacks	callbacks;
		v4_u32					windowBounds;
		const wchar*			windowTitle;
		WindowStyle				windowStyle;
		const wchar*			iconFilePath;
		u32						minThreadsCount;
		const char*				assetsFolderPath;

	};

	Result engine_initialize(const InitializationDesc* desc);
	Result engine_loop();
	Result engine_close();

	Version	engine_version_get() noexcept;
	float	engine_deltatime_get() noexcept;
	u64	engine_frame_count() noexcept;

	void engine_animations_enable();
	void engine_animations_disable();
	bool engine_animations_is_enabled();

}