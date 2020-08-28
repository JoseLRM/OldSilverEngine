#pragma once

namespace sv {

	class Transform {
		EntityTransform* trans;
		ECS* pECS;

	public:
		Transform(Entity entity, EntityTransform* transform, ECS* ecs);
		~Transform() = default;

		Transform(const Transform& other) = default;
		Transform(Transform&& other) = default;

		const Entity entity = 0;

		// getters
		const vec3& GetLocalPosition() const noexcept;
		const vec3& GetLocalRotation() const noexcept;
		const vec3& GetLocalScale() const noexcept;
		XMVECTOR GetLocalPositionDXV() const noexcept;
		XMVECTOR GetLocalRotationDXV() const noexcept;
		XMVECTOR GetLocalScaleDXV() const noexcept;
		XMMATRIX GetLocalMatrix() const noexcept;

		vec3 GetWorldPosition() noexcept;
		vec3 GetWorldRotation() noexcept;
		vec3 GetWorldScale() noexcept;
		XMVECTOR GetWorldPositionDXV() noexcept;
		XMVECTOR GetWorldRotationDXV() noexcept;
		XMVECTOR GetWorldScaleDXV() noexcept;
		XMMATRIX GetWorldMatrix() noexcept;

		inline void* GetInternal() const noexcept { return trans; }

		// setters
		void SetPosition(const vec3& position) noexcept;
		void SetRotation(const vec3& rotation) noexcept;
		void SetScale(const vec3& scale) noexcept;

	private:
		void UpdateWorldMatrix();

	};

}