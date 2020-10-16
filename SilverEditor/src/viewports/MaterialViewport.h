#pragma once

#include "Viewport.h"
#include "high_level/asset_system.h"
#include "TexturePickerViewport.h"

namespace sve {

	class MaterialViewport : public Viewport {

	public:
		MaterialViewport();

	protected:
		bool onDisplay() override;

	private:
		sv::MaterialAsset m_SelectedMaterial;
		TexturePickerViewport m_TexturePicker;

	};

}