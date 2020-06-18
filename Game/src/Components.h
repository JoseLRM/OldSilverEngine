#pragma once

#include <SilverEngine>

SV_COMPONENT(ColisionComponent) {
	float width = 1.f, height = 1.f;
};

SV_COMPONENT(PhysicsComponent){
	static float gravity;
	SV::vec3 vel;
	SV::vec3 acc;
	float mass;
};
