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
		"$(VULKAN_SDK)/Include/",
		"include/",
		"src/"
	}

	-- LIBRARIES

	libdirs {
		"lib/"
	}

	links {
		"$(VULKAN_SDK)/Lib/vulkan-1.lib",
	}
