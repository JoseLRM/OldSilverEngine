-- SILVER ENGINE BUILDING OPTIONS --

-- SV_DEV (DEVELOPER TOOLS)
-- SV_GRAPHICS (GRAPHICS VALIDATION)
-- SV_PATH (CUSTOM PATHS)

_outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

sysPath = ""
resPath = ""

targetdir (binDir .. _outputdir)
objdir (binDir .. _outputdir .. "/int/%{prj.name}")

configurations {

    -- Configs with custom paths
    
    "Slow",		-- Enable graphics validation and dev tools with compiler symbols
    "Debug",		-- Enable dev tools with compiler symbols
    "Graphics",		-- Enable graphics validation and dev tools with compiler optimizations
    "Developer",	-- Enable dev tools with compiler optimizations

    -- Configs without custom paths
    
    "DistributionTest",	-- Developer mode without custom paths
    "Distribution"	-- All disabled with all the optimizations
}

platforms {
    "Win64",
}

-- PLATFORM

filter "platforms:Win64" --win64
architecture "x64"
system "Windows"
systemversion "latest"

defines {
    "SV_PLATFORM_WIN"
}

-- CONFIGURATION

filter "configurations:Slow"
symbols "On"
optimize "Off"
defines {
    "SV_DEV",
    "SV_GRAPHICS",
    "SV_PATH",
    "SV_CONFIG_SLOW"
}

filter "configurations:Debug"
symbols "On"
optimize "Off"
defines {
    "SV_DEV",
    "SV_PATH",
    "SV_CONFIG_DEBUG"
}

filter "configurations:Graphics"
symbols "Off"
optimize "On"
defines {
    "SV_DEV",
    "SV_PATH",
    "SV_GRAPHICS",
    "SV_CONFIG_GRAPHICS"
}

filter "configurations:Developer"
symbols "Off"
optimize "On"
defines {
    "SV_DEV",
    "SV_PATH",
    "SV_CONFIG_DEVELOPER"
}

filter "configurations:DistributionTest"
symbols "Off"
optimize "On"
defines {
    "SV_DEV",
    "SV_CONFIG_DISTRIBUTION",
    "NDEBUG"
}

filter "configurations:Distribution"
symbols "Off"
optimize "On"
defines {
    "SV_CONFIG_DISTRIBUTION",
    "NDEBUG"
}