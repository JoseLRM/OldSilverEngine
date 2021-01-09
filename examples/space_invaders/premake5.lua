workspace 'SpaceInvaders'

    binDir = '%{wks.location}/bin/'
	engineDir = '%{wks.location}/../../'

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

		'SV_RES_PATH=\"resources/\"',
	}

    filter {}
    
    -- MODULES

    include (engineDir .. 'ProjectConfig.lua')
    
	buildDir = (engineDir .. 'modules/build/')
    include (buildDir .. 'sprite.lua')
    include (buildDir .. 'mesh.lua')
    include (buildDir .. 'particle.lua')
    include (buildDir .. 'scene.lua')

    filter 'configurations:not Distribution'
        include(buildDir .. 'editor.lua')
    filter {}
    
    -- GAME PROJECT

    project 'SpaceInvaders'
	    kind 'ConsoleApp'
	    language 'c++'
	    cppdialect 'C++17'

	    files {
	    	'src/**.cpp',
    		'src/**.h'
    	}

	    -- INCLUDE DIRS

       	includedirs {
            (engineDir .. 'modules/include/'),
            '/src/',
    	}

    	-- LIBRARIES

    	links {
    		'SilverEngine',
    	}