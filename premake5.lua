workspace "SilverEngine"

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("%{wks.location}/bin/" .. outputdir)
	objdir ("%{wks.location}/bin/" .. outputdir .. "/int/%{prj.name}")

	configurations {
		"Debug",
		"Release",
		"Distribution"
	}

	platforms {
		"Win64",
	}

	-- DEBUG CONFIG
	
	filter "configurations:not Distribution"

	defines {
		"SV_ENABLE_LOGGING",
		"SV_ENABLE_ASSERTION",
		"SV_ENABLE_VULKAN_VALIDATION_LAYERS",

		"SV_RES_PATH=\"../resources/\"",
	}

	filter {}

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


vulkandir = "C:/VulkanSDK/1.2.141.2/"

include "SilverEngine"
include "SilverEditor"