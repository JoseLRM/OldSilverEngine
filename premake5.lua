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

	-- CONFIG
	
	defines {
		"SV_VULKAN_VALIDATION_LAYERS"
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
			"SV_SRC_PATH=\"../resources/\""
		}

	filter "configurations:Release" --release
		symbols "Off"
		optimize "On"
		defines {
			"SV_RELEASE",
			"SV_SRC_PATH=\"../resources/\""
		}

	filter "configurations:Distribution" --distribution
		symbols "Off"
		optimize "On"
		defines {
			"SV_DIST"
		}
		undefines {
			"SV_VULKAN_VALIDATION_LAYERS"
		}


vulkandir = "C:/VulkanSDK/1.2.141.2/"

include "SilverEngine"
include "SilverEditor"