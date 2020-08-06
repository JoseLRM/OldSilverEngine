#pragma once

#include "..//core.h"
#include "../physics/physics.h"

namespace sv {

	typedef ui16 CompID;
	typedef ui32 Entity;
	typedef void* Scene;

}

constexpr sv::Entity SV_INVALID_ENTITY = 0u;

namespace sv {

	struct BaseComponent {
		Entity entity = SV_INVALID_ENTITY;
	};

	typedef void(*CreateComponentFunction)(BaseComponent*, Entity);
	typedef void(*DestoryComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*CopyComponentFunction)(BaseComponent* from, BaseComponent* to);

	typedef void(*SystemFunction)(Entity entity, BaseComponent** components, float dt);

}

enum SV_ECS_SYSTEM_EXECUTION_MODE : ui8 {
	SV_ECS_SYSTEM_EXECUTION_MODE_SAFE,
	SV_ECS_SYSTEM_EXECUTION_MODE_PARALLEL,
	SV_ECS_SYSTEM_EXECUTION_MODE_MULTITHREADED,
};

struct SV_ECS_SYSTEM_DESC {
	sv::SystemFunction					system;
	SV_ECS_SYSTEM_EXECUTION_MODE		executionMode;
	sv::CompID* pRequestedComponents;
	ui32								requestedComponentsCount;
	sv::CompID* pOptionalComponents;
	ui32								optionalComponentsCount;
};

namespace _sv {

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
		sv::Entity parent = SV_INVALID_ENTITY;
		ui32 childsCount = 0u;
		_sv::EntityTransform transform;
		std::vector<std::pair<sv::CompID, size_t>> indices;

	};

	template<typename T>
	struct Component : public sv::BaseComponent {
		const static sv::CompID ID;
		const static ui32 SIZE;
	};

	struct ECS {
		std::vector<sv::Entity> entities;
		std::vector<EntityData> entityData;
		std::vector<sv::Entity> freeEntityData;
		std::vector<std::vector<ui8>> components;
	};

	struct Scene_internal {
		ECS ecs;
		sv::PhysicsWorld physicsWorld;
		sv::Entity mainCamera;
	};

}

#define SV_COMPONENT(x, ...) struct x; template _sv::Component<x>; struct x : public _sv::Component<x>, __VA_ARGS__