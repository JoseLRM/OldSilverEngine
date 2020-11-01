#pragma once

#include "Panel.h"
#include "simulation/sprite_animator.h"

namespace sv {

	class SpriteAnimationPanel : public Panel {

	public:
		SpriteAnimationPanel();
		~SpriteAnimationPanel();

		inline void setSelectedAnimation(SpriteAnimationAsset& asset) { m_SelectedAnimation = asset; }

	protected:
		bool onDisplay() override;

	private:
		SpriteAnimationAsset m_SelectedAnimation;

	};

}