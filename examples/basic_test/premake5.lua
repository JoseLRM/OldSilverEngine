workspace 'Test'

    binDir = '%{wks.location}/bin/'
	engineDir = '%{wks.location}/../../project/'

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

		'SV_RES_PATH=\"res/\"',
	}

    filter {}

	-- INCLUDE SILVER ENGINE PROJECT

    include (engineDir .. 'ProjectConfig.lua')
    
    -- GAME PROJECT

    project 'Test'
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