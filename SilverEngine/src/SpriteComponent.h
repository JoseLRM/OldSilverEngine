#pragma once

#include "core.h"
#include "Scene.h"

namespace SV {

	struct SpriteComponent : public SV::Component<SpriteComponent> {
		Sprite sprite;

		SpriteComponent() : sprite() {}
		SpriteComponent(const Sprite& spr) : sprite(spr) {}
	};
	SVDefineComponent(SpriteComponent);

}