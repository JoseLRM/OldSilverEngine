#pragma once

#include "entity_system.h"
#include "renderer.h"

namespace sv {

	enum ForceType : ui32 {
		ForceType_Force,
		ForceType_Impulse,
	};

	enum Collider2DType : ui32 {
		Collider2DType_Box,
		Collider2DType_Circle,
	};

	struct CameraSettings {
		bool active = true;
		CameraProjection projection;
	};

}

#include "scene/scene_components.h"

namespace sv {

	struct SceneDesc {
		vec3	gravity;
	};

	struct Scene {
		ECS		ecs;
		Entity	mainCamera;
		float	timeStep;
		void*	pWorld2D;
	};

	struct CameraDrawDesc {
		Offscreen*				pOffscreen;
		const CameraSettings*	pSettings;
		vec3					position;
		vec3					rotation;
	};

	Scene	scene_create(const SceneDesc* desc);
	void	scene_destroy(Scene& scene);
	void	scene_clear(Scene& scene);

	// Rendering

	void scene_draw(Scene& scene);
	void scene_camera_draw(const CameraDrawDesc* desc, Scene& scene);

	// Physics

	void scene_physics_simulate(float dt, Scene& scene);

}