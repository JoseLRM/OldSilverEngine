#pragma once

#include "Viewport.h"
#include "high_level/scene.h"
#include "TexturePickerViewport.h"

namespace sve {

	class EntityInspectorViewport : public Viewport {

	public:
		EntityInspectorViewport();

		void setEntity(sv::Entity entity);

	protected:
		bool onDisplay() override;

	private:
		void showComponentInfo(sv::CompID ID, sv::BaseComponent* comp);

		void showTransform(sv::Transform& transform);

		void showSpriteComponentInfo(sv::SpriteComponent* comp);
		void showNameComponentInfo(sv::NameComponent* comp);
		void showCameraComponentInfo(sv::CameraComponent* comp);
		void showRigidBody2DComponentInfo(sv::RigidBody2DComponent* comp);

	private:
		sv::Entity m_Entity = SV_ENTITY_NULL;
		TexturePickerPopUp m_TexturePicker;

	};

}