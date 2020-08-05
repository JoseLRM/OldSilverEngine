#pragma once

#include "..//core.h"
#include "scene_types.h"

namespace sv {

	class Transform {
		
		_sv::EntityTransform* trans;

	public:
		Transform(Entity entity, _sv::EntityTransform* transform);
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