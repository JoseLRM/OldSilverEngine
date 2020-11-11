#include "core_editor.h"

#include "ImGuiDevice.h"
#include "ImGuiDevice_vk.h"

namespace sv {

	std::unique_ptr<ImGuiDevice> device_create()
	{
		return std::make_unique<ImGuiDevice_vk>();
	}

}