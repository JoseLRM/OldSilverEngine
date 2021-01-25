-- SILVER ENGINE BUILDING OPTIONS --
-- SV_ENABLE_LOGGING
-- SV_ENABLE_ASSERTION
-- SV_ENABLE_PROFILER
-- SV_ENABLE_GFX_VALIDATION

_outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

sysPath = ""
resPath = ""

targetdir (binDir .. _outputdir)
objdir (binDir .. _outputdir .. "/int/%{prj.name}")

configurations {
    "Debug",
    "Release",
    "Distribution"
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

filter "configurations:Debug" --debug
symbols "On"
optimize "Off"
defines {
    "SV_DEBUG",
    
}

filter "configurations:Release" --release
symbols "Off"
optimize "On"
defines {
    "SV_RELEASE",
}

filter "configurations:Distribution" --distribution
symbols "Off"
optimize "On"
defines {
    "SV_DIST",
    "NDEBUG"
}