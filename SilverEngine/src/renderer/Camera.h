#pragma once

#include "..//core.h"
#include "renderer_desc.h"

namespace sv {

	class Camera {
		std::unique_ptr<_sv::Offscreen> m_Offscreen;

	public:
		virtual XMMATRIX GetProjectionMatrix() const = 0;
		virtual XMMATRIX GetViewMatrix() const = 0;
		virtual void Adjust() = 0;

		bool CreateOffscreen(ui32 width, ui32 height);
		bool HasOffscreen() const noexcept;
		_sv::Offscreen& GetOffscreen() noexcept;
		bool DestroyOffscreen();
	};

	class OrthographicCamera : public Camera {
		vec2 m_Position = { 0.f, 0.f };
		vec2 m_Dimension = { 1.f, 1.f };
		float m_Rotation = 0.f;

	public:
		XMMATRIX GetProjectionMatrix() const override;
		XMMATRIX GetViewMatrix() const override;

		void Adjust() override;

		// getters
		inline vec2 GetPosition() const noexcept { return m_Position; }
		inline vec2 GetDimension() const noexcept { return m_Dimension; }
		inline float GetZoom() const noexcept { return m_Dimension.Mag(); }
		inline float GetRotation() const noexcept { return m_Rotation; }
		float GetAspect() const noexcept;

		vec2 GetMousePos();

		// setters
		void SetPosition(vec2 position) noexcept;
		void SetDimension(vec2 dimension) noexcept;
		void SetZoom(float zoom) noexcept;
		void SetRotation(float rotation) noexcept;
		void SetAspect(float aspect) noexcept;

	};

}