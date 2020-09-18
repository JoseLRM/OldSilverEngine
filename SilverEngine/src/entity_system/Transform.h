#pragma once

namespace sv {

	class Transform {
		void* trans;
		ECS* pECS;

	public:
		Transform(Entity entity, void* transform, ECS* ecs);
		~Transform() = default;

		Transform(const Transform& other) = default;
		Transform(Transform&& other) = default;

		const Entity entity = 0;

		// getters
		const vec3f& GetLocalPosition() const noexcept;
		const vec3f& GetLocalRotation() const noexcept;
		const vec3f& GetLocalScale() const noexcept;
		XMVECTOR GetLocalPositionDXV() const noexcept;
		XMVECTOR GetLocalRotationDXV() const noexcept;
		XMVECTOR GetLocalScaleDXV() const noexcept;
		XMMATRIX GetLocalMatrix() const noexcept;

		vec3f GetWorldPosition() noexcept;
		vec3f GetWorldRotation() noexcept;
		vec3f GetWorldScale() noexcept;
		XMVECTOR GetWorldPositionDXV() noexcept;
		XMVECTOR GetWorldRotationDXV() noexcept;
		XMVECTOR GetWorldScaleDXV() noexcept;
		XMMATRIX GetWorldMatrix() noexcept;

		// setters
		void SetPosition(const vec3f& position) noexcept;
		void SetRotation(const vec3f& rotation) noexcept;
		void SetScale(const vec3f& scale) noexcept;

	private:
		void UpdateWorldMatrix();
		void Notify();

	};

}