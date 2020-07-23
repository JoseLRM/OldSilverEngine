#pragma once

#include "..//core.h"

namespace SV {

	class Camera {
	public:
		virtual XMMATRIX GetProjectionMatrix() const = 0;
		virtual XMMATRIX GetViewMatrix() const = 0;
		virtual void Adjust() = 0;
	};

	class OrthographicCamera : public SV::Camera {
		SV::vec2 m_Position = { 0.f, 0.f };
		SV::vec2 m_Dimension = { 1.f, 1.f };
		float m_Rotation = 0.f;

	public:
		XMMATRIX GetProjectionMatrix() const override;
		XMMATRIX GetViewMatrix() const override;

		void Adjust() override;

		// getters
		inline SV::vec2 GetPosition() const noexcept { return m_Position; }
		inline SV::vec2 GetDimension() const noexcept { return m_Dimension; }
		inline float GetRotation() const noexcept { return m_Rotation; }
		float GetAspect() const noexcept;

		SV::vec2 GetMousePos();

		// setters
		void SetPosition(SV::vec2 position) noexcept;
		void SetDimension(SV::vec2 dimension) noexcept;
		void SetRotation(float rotation) noexcept;
		void SetAspect(float aspect) noexcept;

	};

}