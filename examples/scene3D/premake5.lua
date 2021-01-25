workspace 'Scene3D'

	binDir = '%{wks.location}/bin/'
	engineDir = '../../project/'

	include (engineDir .. "SolutionConfig.lua")

	-- BUILDING OPTIONS

	filter 'configurations:Debug'

	defines {
		'SV_ENABLE_GFX_VALIDATION',
	}
	
	filter 'configurations:not Distribution'

	defines {
		'SV_ENABLE_LOGGING',
		'SV_ENABLE_ASSERTION',
	}

	sysPath = engineDir
	resPath = 'res/'

	filter {}

	-- INCLUDE SILVER ENGINE PROJECT

    include (engineDir .. 'ProjectConfig.lua')
    
    -- GAME PROJECT

    project 'Scene3D'
	    kind 'ConsoleApp'
	    language 'c++'
	    cppdialect 'C++17'

	    files {
	    	'src/**.cpp',
    		'src/**.h'
    	}

	    -- INCLUDE DIRS

       	includedirs {
            (engineDir .. 'include/'),
            '/src/',
    	}

    	-- LIBRARIES

    	links {
    		'SilverEngine',
    	}
