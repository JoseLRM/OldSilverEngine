#include "core_editor.h"

#include "EntityInspectorPanel.h"
#include "simulation.h"

#include "gui.h"

namespace sv {

	void showSpriteComponentInfo(SpriteComponent* comp)
	{
		gui_component_item_begin();

		gui_component_item_next("Color");
		gui_component_item_color_picker(comp->color);

		gui_component_item_next("TexCoord");
		ImGui::DragFloat4("##TexCoord", &comp->sprite.texCoord.x, 0.001f);

		gui_component_item_next("Texture");
		gui_component_item_texture(comp->sprite.texture);

		gui_component_item_next("Material");
		gui_component_item_material(comp->material);

		gui_component_item_end();

	}

	void showAnimatedSpriteComponentInfo(AnimatedSpriteComponent* comp)
	{
		static bool simulateAnimation = false;

		gui_component_item_begin();

		gui_component_item_next("Color");
		gui_component_item_color_picker(comp->color);

		gui_component_item_next("Material");
		gui_component_item_material(comp->material);

		gui_component_item_next("Animation");

		SpriteAnimationAsset anim = comp->sprite.getAnimation();
		if (gui_component_item_sprite_animation(anim))
			comp->sprite.setAnimation(anim);

		gui_component_item_next("Sprite Duration");
		float duration = comp->sprite.getSpriteDuration();
		if (ImGui::DragFloat("##Duration", &duration, 0.01f, 0.01f, float_max)) {
			comp->sprite.setSpriteDuration(duration);
		}

		if (anim.hasReference()) {

			{
				gui_component_item_next("State");

				if (comp->sprite.isRunning()) {
					if (ImGui::Button("Pause")) comp->sprite.pause();
				}
				else if (ImGui::Button("Start")) comp->sprite.start();
				ImGui::SameLine();
				if (ImGui::Button("Reset")) comp->sprite.reset();
			}

			{
				gui_component_item_next("Timeline");

				ui32 count = ui32(anim->sprites.size());

				float maxTime = float(count) * duration;
				float currentTime = float(comp->sprite.getIndex()) * duration + comp->sprite.getSpriteTime();

				if (ImGui::SliderFloat("##Timeline", &currentTime, 0.f, maxTime)) {
					ui32 currentIndex = ui32(currentTime / duration);
					float time = currentTime - (currentIndex * duration);
					comp->sprite.setIndex(currentIndex);
					comp->sprite.setSpriteTime(time);
				}
			}

			bool infinite = comp->sprite.getRepeatCount() == 0u;
			{
				gui_component_item_next("Infinite loop");

				if (ImGui::Checkbox("##Infinitelopp", &infinite)) {
					comp->sprite.setRepeatCount(!infinite);
				}
			}

			if (!infinite) {
				gui_component_item_next("Repeat Count");

				int repeat = comp->sprite.getRepeatCount();
				if (ImGui::DragInt("##RepeatCount", &repeat, 1, 1, i32_max)) {
					comp->sprite.setRepeatCount(repeat);
				}

				gui_component_item_next("Repeat Index");

				repeat = comp->sprite.getRepeatIndex();
				if (ImGui::SliderInt("##RepeatIndex", &repeat, 0, comp->sprite.getRepeatCount() - 1u)) {
					comp->sprite.setRepeatIndex(repeat);
				}
			}
		}

		gui_component_item_end();

	}

	void showNameComponentInfo(NameComponent* comp)
	{
		gui_component_item_string(comp->name);
	}

	void showCameraComponentInfo(CameraComponent* comp)
	{
		ProjectionType projectionType = comp->camera.getProjectionType();
		Camera& cam = comp->camera;

		auto getPrjStr = [](ProjectionType type) {
			switch (type)
			{
			case sv::ProjectionType_Clip:
				return "Clip";
			case sv::ProjectionType_Orthographic:
				return "Orthographic";
			case sv::ProjectionType_Perspective:
				return "Perspective";
			default:
				return "None";
			}
		};

		auto getPrjNearMin = [](ProjectionType type) {
			switch (type)
			{
			case sv::ProjectionType_Orthographic:
				return float_min;
			case sv::ProjectionType_Perspective:
				return 0.001f;
			default:
				return float_min;
			}
		};

		gui_component_item_begin();

		ProjectionType types[] = {
			ProjectionType_Clip,
			ProjectionType_Orthographic,
			ProjectionType_Perspective
		};
	
		gui_component_item_next("Projection");
		if (ImGui::BeginCombo("##Projection", getPrjStr(cam.getProjectionType()))) {

			for (ui32 i = 0; i < 3; ++i) {
				ProjectionType type = types[i];

				if (cam.getProjectionType() == type) continue;

				if (ImGui::MenuItem(getPrjStr(type))) {
					cam.setProjectionType(type);

					// Initialize values
					switch (type)
					{
					case sv::ProjectionType_Clip:
						break;
					case sv::ProjectionType_Orthographic:
						cam.setNear(-1000.f);
						cam.setFar(1000.f);
						cam.setProjectionLength(10.f);
						break;
					case sv::ProjectionType_Perspective:
						cam.setNear(0.1f);
						cam.setFar(1000.f);
						cam.setProjectionLength(0.1f);
						break;
					}
				}
			}

			ImGui::EndCombo();
		}

		gui_component_item_next("Length");
		float length = cam.getProjectionLength();
		if (ImGui::DragFloat("##Length", &length, 0.1f, 0.01f, float_max)) {
			cam.setProjectionLength(length);
		}

		gui_component_item_next("Near");
		float near = cam.getNear();
		if (ImGui::DragFloat("##Near", &near, 0.1f, getPrjNearMin(cam.getProjectionType()), float_max)) {
			cam.setNear(near);
		}

		gui_component_item_next("Far");
		float far = cam.getFar();
		if (ImGui::DragFloat("##Far", &far, 0.1f, near + 0.001f, float_max)) {
			cam.setFar(far);
		}
		
		gui_component_item_next("Resolution");
		vec2u res = cam.getResolution();
		vec2i res0 = { i32(res.x), i32(res.y) };
		if (ImGui::DragInt2("##Res", &res0.x, 1, 10, 2000)) {
			//cam.setResolution(res0.x, res0.y);
			SV_LOG_ERROR("TODO");
		}

		gui_component_item_next("Active");
		bool active = cam.isActive();
		if (ImGui::Checkbox("##Active", &active)) {
			if (active) cam.activate();
			else cam.deactivate();
		}

		gui_component_item_next("Main Camera");
		bool isMain = g_Scene->getMainCamera() == comp->entity;
		if (ImGui::Checkbox("##MainCamera", &isMain)) {
			if (isMain) {
				g_Scene->setMainCamera(comp->entity);
			}
			else g_Scene->setMainCamera(SV_ENTITY_NULL);
		}

		gui_component_item_end();
	}

	void showRigidBody2DComponentInfo(RigidBody2DComponent* comp)
	{
		gui_component_item_begin();

		{
			bool dynamic = comp->isDynamic();
			gui_component_item_next("Dynamic");
			if (ImGui::Checkbox("##Dynamic", &dynamic)) comp->setDynamic(dynamic);
		}
		{
			bool fixedRot = comp->isFixedRotation();
			gui_component_item_next("Fixed Rotation");
			if (ImGui::Checkbox("##FixedRot", &fixedRot)) comp->setFixedRotation(fixedRot);
		}
		{
			gui_component_item_next("Velocity");
			vec2f vel = comp->getVelocity();
			if (ImGui::DragFloat2("##Velocity", &vel.x, 0.1f)) comp->setVelocity(vel);
		}
		{
			gui_component_item_next("Angular Velocity");
			float vel = comp->getAngularVelocity();
			if (ImGui::DragFloat("##AngularVelocity", &vel, 0.1f)) comp->setAngularVelocity(vel);
		}

		gui_component_item_end();
	}

	void showBoxCollider2DComponentInfo(BoxCollider2DComponent* comp)
	{
		gui_component_item_begin();

		gui_component_item_next("Density");
		float density = comp->getDensity();
		if (ImGui::DragFloat("##density", &density, 0.1f, 0.f)) comp->setDensity(density);

		gui_component_item_next("Friction");
		float friction = comp->getFriction();
		if (ImGui::DragFloat("##friction", &friction, 0.1f, 0.f)) comp->setFriction(friction);

		gui_component_item_next("Restitution");
		float restitution = comp->getRestitution();
		if (ImGui::DragFloat("##restitution", &restitution, 0.01f, 0.f)) comp->setRestitution(restitution);

		gui_component_item_next("Size");
		vec2f size = comp->getSize();
		if (ImGui::DragFloat2("##size", &size.x, 0.1f)) comp->setSize(size);

		gui_component_item_next("Offset");
		vec2f offset = comp->getOffset();
		if (ImGui::DragFloat2("##offset", &offset.x, 0.01f)) comp->setOffset(offset);

		gui_component_item_end();
	}

	void showCircleCollider2DComponentInfo(CircleCollider2DComponent* comp)
	{
		gui_component_item_begin();

		gui_component_item_next("Density");
		float density = comp->getDensity();
		if (ImGui::DragFloat("##density", &density, 0.1f, 0.f)) comp->setDensity(density);

		gui_component_item_next("Friction");
		float friction = comp->getFriction();
		if (ImGui::DragFloat("##friction", &friction, 0.1f, 0.f)) comp->setFriction(friction);

		gui_component_item_next("Restitution");
		float restitution = comp->getRestitution();
		if (ImGui::DragFloat("##restitution", &restitution, 0.01f, 0.f)) comp->setRestitution(restitution);

		gui_component_item_next("Radius");
		float radius = comp->getRadius();
		if (ImGui::DragFloat("##radius", &radius, 0.1f)) comp->setRadius(radius);

		gui_component_item_next("Offset");
		vec2f offset = comp->getOffset();
		if (ImGui::DragFloat2("##offset", &offset.x, 0.01f)) comp->setOffset(offset);

		gui_component_item_end();
	}

	void showComponentInfo(CompID ID, BaseComponent* comp)
	{
		if (ID == SpriteComponent::ID)					showSpriteComponentInfo(reinterpret_cast<SpriteComponent*>(comp));
		else if (ID == AnimatedSpriteComponent::ID)		showAnimatedSpriteComponentInfo(reinterpret_cast<AnimatedSpriteComponent*>(comp));
		else if (ID == NameComponent::ID)				showNameComponentInfo(reinterpret_cast<NameComponent*>(comp));
		else if (ID == CameraComponent::ID)				showCameraComponentInfo(reinterpret_cast<CameraComponent*>(comp));
		else if (ID == RigidBody2DComponent::ID)		showRigidBody2DComponentInfo(reinterpret_cast<RigidBody2DComponent*>(comp));
		else if (ID == BoxCollider2DComponent::ID)		showBoxCollider2DComponentInfo(reinterpret_cast<BoxCollider2DComponent*>(comp));
		else if (ID == CircleCollider2DComponent::ID)	showCircleCollider2DComponentInfo(reinterpret_cast<CircleCollider2DComponent*>(comp));
	}

	EntityInspectorPanel::EntityInspectorPanel()
	{

	}

	bool EntityInspectorPanel::onDisplay()
	{
		ECS* ecs = g_Scene->getECS();

		if (g_SelectedEntity != SV_ENTITY_NULL) {

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

			NameComponent* nameComponent = ecs_component_get<NameComponent>(ecs, g_SelectedEntity);
			if (nameComponent) {
				ImGui::Text(nameComponent->name.c_str(), nameComponent->name.c_str());
			}
			else {
				ImGui::Text("Entity");
			}

			ImGui::PopFont();

			ImGui::Separator();


			// Show Transform Data
			gui_transform_show(ecs_entity_transform_get(ecs, g_SelectedEntity));

			// Show components
			for (ui32 i = 0; i < ecs_entity_component_count(ecs, g_SelectedEntity); ++i) {

				auto [compID, comp] = ecs_component_get_by_index(ecs, g_SelectedEntity, i);
				gui_component_show(ecs, compID, comp, showComponentInfo);
				
			}


			if (ImGui::BeginCombo("##Add", "Add Component")) {

				for (ui16 ID = 0; ID < ecs_component_register_count(); ++ID) {

					if (!ecs_register_exist(ecs, ID)) continue;

					const char* NAME = ecs_component_name(ID);
					if (ecs_component_get_by_id(ecs, g_SelectedEntity, ID) != nullptr) continue;
					size_t SIZE = ecs_component_size(ID);
					if (ImGui::Button(NAME)) {
						ecs_component_add_by_id(ecs, g_SelectedEntity, ID);
					}
				}

				ImGui::EndCombo();
			}
			if (ImGui::BeginCombo("##Rmv", "Remove Component")) {

				for (ui32 i = 0; i < ecs_entity_component_count(ecs, g_SelectedEntity); ++i) {
					CompID ID = ecs_component_get_by_index(ecs, g_SelectedEntity, i).first;
					const char* NAME = ecs_component_name(ID);
					size_t SIZE = ecs_component_size(ID);
					if (ImGui::Button(NAME)) {
						ecs_component_remove_by_id(ecs, g_SelectedEntity, ID);
						break;
					}
				}

				ImGui::EndCombo();
			}

		}

		return true;
	}

}