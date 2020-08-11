#pragma once

#include "scene.h"
#include "renderer/renderer_internal.h"
#include "physics/physics_internal.h"

namespace sv {

	struct EntityTransform {

		XMFLOAT3 localPosition = { 0.f, 0.f, 0.f };
		XMFLOAT3 localRotation = { 0.f, 0.f, 0.f };
		XMFLOAT3 localScale = { 1.f, 1.f, 1.f };

		XMFLOAT4X4 worldMatrix;

		bool modified = true;
		bool wakePhysics = true;

	};

	struct EntityData {

		size_t handleIndex = 0u;
		Entity parent = SV_INVALID_ENTITY;
		ui32 childsCount = 0u;
		EntityTransform transform;
		std::vector<std::pair<CompID, size_t>> indices;

	};

	struct ECS {
		std::vector<Entity> entities;
		std::vector<EntityData> entityData;
		std::vector<Entity> freeEntityData;
		std::vector<std::vector<ui8>> components;
	};

	struct Scene_internal {
		ECS ecs;
		PhysicsWorld physicsWorld;
		RenderWorld renderWorld;
		Entity mainCamera;
	};

	std::vector<EntityData>& scene_ecs_get_entity_data();
	EntityData& scene_ecs_get_entity_data(sv::Entity entity);

	PhysicsWorld& scene_physicsWorld_get();
	RenderWorld& scene_renderWorld_get();

}