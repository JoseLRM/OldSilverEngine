#pragma once

#include "core.h"
#include "RenderQueue2D.h"
#include "Renderer2D.h"
#include "PostProcess.h"
#include "Camera.h"

struct SV_RENDERER_INITIALIZATION_DESC {

};

namespace SV {

	class Renderer : public SV::EngineDevice {
		RenderQueue2D m_RenderQueue2D;
		
		Renderer2D m_Renderer2D;
		PostProcess m_PostProcess;

		FrameBuffer m_Offscreen;

		SV::Camera* m_pCamera;

		Renderer();
		~Renderer();

		bool Initialize(SV_RENDERER_INITIALIZATION_DESC& desc);
		bool Close();

	public:
		friend Engine;

		inline RenderQueue2D& GetRenderQueue2D() noexcept { return m_RenderQueue2D; }

		void BeginFrame();
		void Render();
		void EndFrame();

		inline SV::Camera* GetCamera() const noexcept { return m_pCamera; };
		inline void SetCamera(SV::Camera* camera) noexcept { m_pCamera = camera; };

	};

}