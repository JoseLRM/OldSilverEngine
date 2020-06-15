#pragma once

#include "Components.h"

class ColisionSystem : public SV::System {

public:
	ColisionSystem();

	void UpdateEntity(SV::Entity entity, SV::BaseComponent** comp, SV::Scene& scene, float dt) override;

};