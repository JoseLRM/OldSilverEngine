project 'SilverEngine'
	
	-- FILES

	files {
        '../src/' .. _moduleName .. '/**.cpp',
		'../src/' .. _moduleName .. '/**.hpp',
		'../src/' .. _moduleName .. '/**.c',
		'../src/' .. _moduleName .. '/**.h',
        '../include/' .. _moduleName .. '/**.cpp',
		'../include/' .. _moduleName .. '/**.hpp',
		'../include/' .. _moduleName .. '/**.c',
		'../include/' .. _moduleName .. '/**.h',
	}

	-- PRECOMILED HEADER

	filter {
		'files:../src/' .. _moduleName .. '/external/**',
	}
		flags { 'NoPCH' }
	filter {}

	-- DEFINES

	project '*'
    
	defines {
		('SV_MODULE_' .. _moduleName:upper())
    }
    
project 'SilverEngine'