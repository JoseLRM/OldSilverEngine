#include "ColisionSystem.h"

ColisionSystem::ColisionSystem()
{
	AddRequestedComponent(PhysicsComponent::ID);
}

void ColisionSystem::UpdateEntity(SV::Entity entity, SV::BaseComponent** comp, SV::Scene& scene, float dt)
{
	PhysicsComponent* physics = reinterpret_cast<PhysicsComponent*>(comp[0]);
	SV::Transform& trans = scene.GetTransform(physics->entity);

	// Simulation
	{
		SV::vec3 pos = trans.GetLocalPosition();
		SV::vec3& vel = physics->vel;
		SV::vec3& acc = physics->acc;

		vel += acc * dt;
		pos += vel * dt;
		acc = acc * 0.9f * dt;

		trans.SetPosition(pos);
	}

	// Gravity
	physics->vel.y -= 10.f * dt;

	// Air frinction
	physics->vel.y -= physics->vel.y * 0.1f * dt;

	// Colisions
	auto& colisions = scene.GetComponentsList(ColisionComponent::ID);

	SV::vec2 pos = SV::vec2(trans.GetWorldPosition()) + physics->vel * dt;
	SV::vec2 size = SV::vec2(trans.GetWorldScale()) / 2.f;

	for (ui32 i = 0; i < colisions.size(); i += ColisionComponent::SIZE) {
		ColisionComponent* col = reinterpret_cast<ColisionComponent*>(&colisions[i]);
		SV::Transform& colTrans = scene.GetTransform(col->entity);
		
		SV::vec2 colPos = SV::vec2(colTrans.GetWorldPosition());
		SV::vec2 colSize = SV::vec2(colTrans.GetWorldScale()) / 2.f;

		SV::vec2 dist = pos - colPos;
		SV::vec2 minDist = size + colSize;

		dist.x = abs(dist.x);
		dist.y = abs(dist.y);

		// Detection
		if (dist.x <= minDist.x && dist.y <= minDist.y) {

			minDist -= dist;

			if (minDist.x <= minDist.y) {
				minDist.y = 0.f;
				physics->vel.x = 0.f;
				if (pos.x < colPos.x) minDist.x = -minDist.x;
			}
			else {
				minDist.x = 0.f;
				physics->vel.y = 0.f;
				if (pos.y < colPos.y) minDist.y = -minDist.y;
			}

			trans.SetPosition(trans.GetLocalPosition() + SV::vec3(minDist.x, minDist.y, 0.f));
			pos = SV::vec2(trans.GetWorldPosition()) + physics->vel * dt;

		}
	}
}
