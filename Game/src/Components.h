#pragma once

#include <SilverEngine>

struct ColisionComponent : public SV::Component<ColisionComponent> {
	float width = 1.f, height = 1.f;

#ifdef SV_IMGUI
	void ShowInfo() override
	{
		ImGui::DragFloat("Size", &width, 0.01f);
	}
#endif
};
SVDefineComponent(ColisionComponent);

struct PhysicsComponent : public SV::Component<PhysicsComponent> {
	SV::vec3 vel;
	SV::vec3 acc;
	float mass;
};
SVDefineComponent(PhysicsComponent);