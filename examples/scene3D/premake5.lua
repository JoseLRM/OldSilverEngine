workspace 'Scene3D'

	binDir = '%{wks.location}/bin/'
	engineDir = '../../project/'

	include (engineDir .. "SolutionConfig.lua")

	-- PATHS

	sysPath = engineDir
	resPath = 'res/'

	defines {
		'SV_RES_PATH=\"' .. resPath .. '\"',
		'SV_SYS_PATH=\"' .. sysPath .. '\"',
	}

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
