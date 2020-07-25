#pragma once

#include "..//core.h"

namespace SV {

	class Scene;
	struct BaseComponent;

	typedef ui16 CompID;
	typedef ui32 Entity;

	typedef void(*CreateComponentFunction)(BaseComponent*, SV::Entity);
	typedef void(*DestoryComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*CopyComponentFunction)(BaseComponent* from, BaseComponent* to);

	typedef void(*SystemFunction)(Scene& scene, Entity entity, BaseComponent** components, float dt);

}

constexpr SV::Entity SV_INVALID_ENTITY = 0u;

enum SV_ECS_SYSTEM_EXECUTION_MODE : ui8 {
	SV_ECS_SYSTEM_EXECUTION_MODE_SAFE,
	SV_ECS_SYSTEM_EXECUTION_MODE_PARALLEL,
	SV_ECS_SYSTEM_EXECUTION_MODE_MULTITHREADED,
};

struct SV_ECS_SYSTEM_DESC {
	SV::SystemFunction					system;
	SV_ECS_SYSTEM_EXECUTION_MODE		executionMode;
	SV::CompID* pRequestedComponents;
	ui32								requestedComponentsCount;
	SV::CompID* pOptionalComponents;
	ui32								optionalComponentsCount;
};
