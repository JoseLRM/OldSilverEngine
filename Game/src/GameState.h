#pragma once

#include "engine.h"
#include "components.h"

void System(sv::Entity entity, sv::BaseComponent** comp, float dt)
{
	sv::SpriteComponent& sprComp = *reinterpret_cast<sv::SpriteComponent*>(comp[0]);
	sv::Transform trans = sv::scene_ecs_entity_get_transform(entity);
	sv::vec3 rot = trans.GetLocalRotation();

	ui32 type = entity % 3;

	switch (type)
	{
	case 0:
		rot.z += dt * ToRadians(5.f);
		break;
	case 1:
		rot.z -= dt * ToRadians(50.f);
		break;
	case 2:
		rot.z += dt * ToRadians(180.f);
		break;
	}

	trans.SetRotation(rot);

}

class GameState : public sv::State {

	sv::Entity cameraEntity;
	sv::Scene scene;
	sv::Entity entity;

	sv::TextureAtlas texture;
	sv::Sprite sprite;

	void createEntity()
	{
		constexpr ui32 count = 1;
		sv::Entity entities[count];

		sv::scene_ecs_entities_create(count, 0u, entities);
		sv::scene_ecs_components_add<sv::SpriteComponent>(entities, count, sprite);

		sv::CameraComponent* camera = sv::scene_ecs_component_get<sv::CameraComponent>(cameraEntity);
		sv::vec3 cameraPos = sv::scene_ecs_entity_get_transform(cameraEntity).GetWorldPosition();

		for (ui32 i = 0; i < count; ++i) {
			sv::Transform trans = sv::scene_ecs_entity_get_transform(entities[i]);
			sv::SpriteComponent& sprComp = *sv::scene_ecs_component_get<sv::SpriteComponent>(entities[i]);

			sv::vec3 pos;
			sv::vec2 mousePos = sv::renderer_compute_orthographic_position(camera->projection, sv::input_mouse_position_get());
			mousePos.x += cameraPos.x;
			mousePos.y += cameraPos.y;
			pos.x = mousePos.x;
			pos.y = mousePos.y;
			//pos.x = (float(rand()) / RAND_MAX) * 800.f - 400.f;
			//pos.y = (float(rand()) / RAND_MAX) * 400.f - 200.f;

			sv::vec3 scale;
			scale.x = (float(rand()) / RAND_MAX) + 0.1f;
			scale.y = (float(rand()) / RAND_MAX) + 0.1f;

			sv::vec3 rot;
			rot.z = ToRadians((float(rand()) / RAND_MAX) * 360.f);

			sprComp.color.x = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.y = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.z = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.w = 255u;

			trans.SetPosition(pos);
			trans.SetScale(scale);
			trans.SetRotation(rot);

		}
	}

	void createEntities()
	{
		constexpr ui32 count = 1000;
		sv::Entity entities[count];

		sv::scene_ecs_entities_create(count, 0u, entities);
		sv::scene_ecs_components_add<sv::SpriteComponent>(entities, count, sprite);

		for (ui32 i = 0; i < count; ++i) {
			sv::Transform trans = sv::scene_ecs_entity_get_transform(entities[i]);
			sv::SpriteComponent& sprComp = *sv::scene_ecs_component_get<sv::SpriteComponent>(entities[i]);

			sv::vec3 pos;
			sv::vec3 scale;
			sv::vec3 rot;

			//#define TEST0

#ifdef TEST0
			sv::vec2 mousePos = camera.GetMousePos();
			pos.x = mousePos.x;
			pos.y = mousePos.y;

			scale.x = 1.f;
			scale.y = 1.f;
#else
			pos.x = (float(rand()) / RAND_MAX) * 400.f - 300.f;
			pos.y = (float(rand()) / RAND_MAX) * 200.f - 100.f;

			scale.x = (float(rand()) / RAND_MAX) + 0.1f;
			scale.y = (float(rand()) / RAND_MAX) + 0.1f;

			rot.z = ToRadians((float(rand()) / RAND_MAX) * 360.f);

			sprComp.color.x = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.y = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.z = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.w = 100u;
#endif

			trans.SetPosition(pos);
			trans.SetScale(scale);
			trans.SetRotation(rot);

		}
	}

public:
	void Load() override
	{
		
	}
	void Initialize() override 
	{
		scene = sv::scene_create();
		sv::scene_bind(scene);

		cameraEntity = sv::scene_ecs_entity_create();
		sv::scene_ecs_component_add<sv::CameraComponent>(cameraEntity, sv::CameraType_Orthographic);
		sv::scene_ecs_component_get<sv::CameraComponent>(cameraEntity)->projection.orthographic = { 10.f, 10.f };

		sv::scene_camera_set(cameraEntity);

		sv::textureAtlas_create_from_file("res/Tileset.png", false, sv::SamplerAddressMode_Wrap, &texture);
		sprite = sv::textureAtlas_sprite_add(texture, 0.1f, 0.f, 0.1f, 1.f / 6.f);

		entity = sv::scene_ecs_entity_create();
		sv::scene_ecs_component_add<sv::SpriteComponent>(entity, sprite, sv::Color{0u,255u,255u,100u});
	}
	void Update(float dt) override
	{
		{
			sv::CameraComponent* cameraComp = sv::scene_ecs_component_get<sv::CameraComponent>(cameraEntity);
			cameraComp->Adjust(sv::window_get_width(), sv::window_get_height());

			sv::CameraProjection& camera = cameraComp->projection;

			sv::vec2 dir;
			float dirZoom = 0u;
			float add = 7.f * dt * sv::renderer_compute_orthographic_zoom_get(camera) * 0.05f;
			float addZoom = dt * 10.f * (sv::renderer_compute_orthographic_zoom_get(camera) * 0.05f);

			if (sv::input_key('W')) {
				dir.y += add;
			}
			if (sv::input_key('S')) {
				dir.y -= add;
			}
			if (sv::input_key('D')) {
				dir.x += add;
			}
			if (sv::input_key('A')) {
				dir.x -= add;
			}
			if (sv::input_key(SV_KEY_SPACE)) {
				dirZoom += addZoom;
			}
			if (sv::input_key(SV_KEY_SHIFT)) {
				dirZoom -= addZoom;
			}
			sv::renderer_compute_orthographic_zoom_set(camera, sv::renderer_compute_orthographic_zoom_get(camera) + dirZoom);

			sv::Transform trans = sv::scene_ecs_entity_get_transform(cameraEntity);
			trans.SetPosition(trans.GetLocalPosition() + dir);
		}

		{
			sv::Transform trans = sv::scene_ecs_entity_get_transform(entity);
			sv::vec3 rot = trans.GetLocalRotation();

			if (sv::input_key('I')) {
				rot.x += dt * 2.f;
			}
			if (sv::input_key('O')) {
				rot.y += dt * 2.f;
			}
			if (sv::input_key('P')) {
				rot.z += dt * 2.f;
			}

			trans.SetRotation(rot);
		}

		if (sv::input_mouse_pressed(SV_MOUSE_CENTER)) {
			createEntities();
		}

		sv::CompID req[] = {
			sv::SpriteComponent::ID
		};
		sv::SystemDesc system;

		system.executionMode = sv::SystemExecutionMode_Safe;
		system.optionalComponentsCount = 0u;
		system.requestedComponentsCount = 1u;
		system.system = System;
		system.pRequestedComponents = req;
		if (sv::input_mouse(SV_MOUSE_RIGHT))
			sv::scene_ecs_system_execute(&system, 1u, dt);

		if (sv::input_key_pressed(SV_KEY_ESCAPE)) sv::engine_request_close();
		if (sv::input_key_pressed('F')) sv::window_fullscreen_set(!sv::window_fullscreen_get());
	}
	void Render() override
	{
		sv::CameraComponent* cameraComp = sv::scene_ecs_component_get<sv::CameraComponent>(cameraEntity);
		sv::Transform trans = sv::scene_ecs_entity_get_transform(cameraEntity);

		sv::renderer_scene_begin();
		sv::renderer_scene_draw_scene();
		sv::renderer_scene_end();
	}
	void Unload() override
	{
	}
	void Close() override
	{
		sv::textureAtlas_destroy(texture);
	}

};