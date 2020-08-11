#pragma once

namespace sv {

	class Transform {
		void* trans;

	public:
		Transform(Entity entity, void* transform);
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

		void SetWorldTransformPRS(const vec3& position, const vec3& rotation, const vec3& scale) noexcept;
		void SetWorldTransformPR(const vec3& position, const vec3& rotation) noexcept;

	private:
		void UpdateWorldMatrix();

	};

}