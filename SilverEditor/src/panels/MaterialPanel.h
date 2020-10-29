#pragma once

#include "Panel.h"
#include "rendering/material_system.h"

namespace sv {

	class MaterialPanel : public Panel {

	public:
		MaterialPanel();

		inline void setSelectedMaterial(MaterialAsset& asset) { m_SelectedMaterial = asset; }

	protected:
		bool onDisplay() override;

	private:
		MaterialAsset m_SelectedMaterial;

	};

}