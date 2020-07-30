#pragma once

#include "..//core.h"
#include "SceneTypes.h"

namespace _sv {

	struct EntityTransform {
		XMFLOAT3 localPosition = { 0.f, 0.f, 0.f };
		XMFLOAT3 localRotation = { 0.f, 0.f, 0.f };
		XMFLOAT3 localScale = { 1.f, 1.f, 1.f };

		XMFLOAT4X4 worldMatrix;

		bool modified = true;
	};

}

namespace sv {

	class Transform {
		
		_sv::EntityTransform* trans;
		Scene* scene;

	public:
		Transform(Entity entity, _sv::EntityTransform* transform, Scene* pScene);
		~Transform() = default;

		Transform(const Transform& other) = default;
		Transform(Transform&& other) = default;

		void operator=(const Transform& other);
		void operator=(Transform&& other) = delete;

		const Entity entity = 0;

		// getters
		inline const vec3& GetLocalPosition() const noexcept { return *(vec3*)& trans->localPosition; }
		inline const vec3& GetLocalRotation() const noexcept { return *(vec3*)& trans->localRotation; }
		inline const vec3& GetLocalScale() const noexcept { return *(vec3*)& trans->localScale; }
		inline XMVECTOR GetLocalPositionDXV() const noexcept { return XMLoadFloat3(&trans->localPosition); }
		inline XMVECTOR GetLocalRotationDXV() const noexcept { return XMLoadFloat3(&trans->localRotation); }
		inline XMVECTOR GetLocalScaleDXV() const noexcept { return XMLoadFloat3(&trans->localScale); }
		XMMATRIX GetLocalMatrix() const noexcept;

		vec3 GetWorldPosition() noexcept;
		vec3 GetWorldRotation() noexcept;
		vec3 GetWorldScale() noexcept;
		XMVECTOR GetWorldPositionDXV() noexcept;
		XMVECTOR GetWorldRotationDXV() noexcept;
		XMVECTOR GetWorldScaleDXV() noexcept;
		XMMATRIX GetWorldMatrix() noexcept;

		// setters
		void SetPosition(const vec3& position) noexcept;
		void SetRotation(const vec3& rotation) noexcept;
		void SetScale(const vec3& scale) noexcept;

	private:
		void UpdateWorldMatrix();

	};

}