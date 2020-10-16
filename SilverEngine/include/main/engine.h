#pragma once

#include "core.h"

#include "platform/window.h"
#include "utils/Version.h"

// Main engine

namespace sv {

	struct ApplicationCallbacks {
		Result(*initialize)();
		void(*update)(float);
		void(*render)();
		Result(*close)();
	};

	struct InitializationDesc {

		ApplicationCallbacks callbacks;
		vec4u			windowBounds;
		const wchar*	windowTitle;
		WindowStyle		windowStyle;
		const wchar*	iconFilePath;
		bool			consoleShow;
		const char*		logFolder;
		ui32			minThreadsCount;
		const char*		assetsFolderPath;

	};

	Result engine_initialize(const InitializationDesc* desc);
	Result engine_loop();
	Result engine_close();

	Version	engine_version_get() noexcept;
	float	engine_deltatime_get() noexcept;
	ui64	engine_stats_get_frame_count() noexcept;

}