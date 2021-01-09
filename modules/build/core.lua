project 'SilverEngine'
	
	_moduleName = "core"
	dofile ("_module.lua")

	-- FILES

	files {
		'../include/core.h'
	}

	-- INCLUDE DIRS

	includedirs {
		'$(VULKAN_SDK)/Include/',
	}

	-- LIBS

	links {
		'$(VULKAN_SDK)/Lib/vulkan-1.lib',
	}