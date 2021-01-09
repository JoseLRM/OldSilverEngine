project 'SilverEngine'
	kind 'StaticLib'
	language 'c++'
	cppdialect 'C++17'

	_includeDir = 	'modules/include/'
	_srcDir = 		'modules/src/'
	_libDir = 		'modules/lib/'
	
	-- PROJECT LOCATION
	location (engineDir .. 'modules')

	-- INCLUDE DIRS

	includedirs {
		_includeDir,
		_srcDir
	}

	-- LIBS

	libdirs {
		_libDir
	}

	-- PRECOMILED HEADER
	pchheader 'core.h'
	pchsource 'modules/src/core/core.cpp'

	-- INCLUDE CORE MODULE
	
	include 'modules/build/core.lua'