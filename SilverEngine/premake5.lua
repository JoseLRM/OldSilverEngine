project "SilverEngine"
	kind "StaticLib"
	language "c++"
	cppdialect "C++17"

	files {
		"src/**.cpp",
		"src/**.h",
		"include/**.cpp",
		"include/**.h"
	}

	-- PRECOMILED HEADER

	filter {
		"files:src/external/**",
	}
		flags { "NoPCH" }
	filter {}

	pchheader "core.h"
	pchsource "%{prj.location}/src/core.cpp"

	-- INCLUDE DIRS

	includedirs {
		(vulkandir .. "/Include/"),
		"include/",
		"src/"
	}

	-- LIBRARIES

	libdirs {
		(vulkandir .. "Lib/"),
		"lib/"
	}

	links {
		"vulkan-1.lib"
	}

	filter "configurations:Debug"
		links {
			"box2d_debug.lib"
		}

	filter "configurations:Release or configurations:Distribution"
		links {
			"box2d_release.lib"
		}