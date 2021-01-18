project 'SilverEngine'
	kind 'StaticLib'
	language 'c++'
	cppdialect 'C++17'

	-- INCLUDE DIRS

	includedirs {
		"include/",
		"src/",
		'$(VULKAN_SDK)/Include/',
	}

	-- LIB DIRS

	libdirs {
		"lib/"
	}

	-- PRECOMILED HEADER
	pchheader 'SilverEngine/core.h'
	pchsource 'src/core.cpp'

	-- FILES
	files {
        'src/**.cpp',
		'src/**.hpp',
		'src/**.c',
		'src/**.h',
        'include/**.cpp',
		'include/**.hpp',
		'include/**.c',
		'include/**.h',
	}

	-- PRECOMILED HEADER

	filter {
		'files:src/external/**',
	}
		flags { 'NoPCH' }
	filter {}

	-- LIBS

	links {
		'$(VULKAN_SDK)/Lib/vulkan-1.lib',
	}