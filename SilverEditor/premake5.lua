project "SilverEditor"
	kind "ConsoleApp"
	language "c++"
	cppdialect "C++17"

	files {
		"src/**.cpp",
		"src/**.h"
	}

	-- PRECOMILED HEADER

	filter {
		"files:src/external/**",
	}
		flags { "NoPCH" }
	filter {}

	pchheader "core_editor.h"
	pchsource "%{prj.location}/src/Main.cpp"

	-- INCLUDE DIRS

	includedirs {
		(vulkandir .. "/Include/"),
		"%{wks.location}/SilverEngine/include/",
		"%{prj.location}/src/"
	}

	-- LIBRARIES

	libdirs {
		(vulkandir .. "Lib/")
	}

	links {
		"SilverEngine",
		"vulkan-1.lib"
	}