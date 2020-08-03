#include "core.h"

#include "ImGuiDevice.h"
#include "ImGuiDevice_vk.h"

namespace sve {

	std::unique_ptr<ImGuiDevice> device_create()
	{
		return std::make_unique<ImGuiDevice_vk>();
	}

}