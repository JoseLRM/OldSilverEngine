#pragma once

#include "core.h"

namespace SV {

	class Scene;
	typedef ui32 Entity;

	struct Transform {

	private:
		XMFLOAT3 m_LocalPosition = { 0.f, 0.f, 0.f };
		XMFLOAT3 m_LocalRotation = { 0.f, 0.f, 0.f };
		XMFLOAT3 m_LocalScale = { 1.f, 1.f, 1.f };

		XMFLOAT4X4 m_WorldMatrix;

		bool m_Modified = true;

		Scene* m_pScene;

	public:
		Transform(SV::Entity entity, Scene* pScene);
		~Transform() = default;

		Transform(const Transform& other) = default;
		Transform(Transform&& other) = default;

		void operator=(const Transform& other);
		void operator=(Transform&& other);

		Entity entity = 0;

		// getters
		inline const vec3& GetLocalPosition() const noexcept { return *(vec3*)& m_LocalPosition; }
		inline const vec3& GetLocalRotation() const noexcept { return *(vec3*)& m_LocalRotation; }
		inline const vec3& GetLocalScale() const noexcept { return *(vec3*)& m_LocalScale; }
		inline XMVECTOR GetLocalPositionDXV() const noexcept { return XMLoadFloat3(&m_LocalPosition); }
		inline XMVECTOR GetLocalRotationDXV() const noexcept { return XMLoadFloat3(&m_LocalRotation); }
		inline XMVECTOR GetLocalScaleDXV() const noexcept { return XMLoadFloat3(&m_LocalScale); }
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