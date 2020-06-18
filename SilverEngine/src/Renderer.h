#pragma once

#include "core.h"
#include "RenderQueue2D.h"
#include "Renderer2D.h"
#include "PostProcess.h"
#include "Camera.h"
#include "Renderer2DComponents.h"

struct SV_RENDERER_INITIALIZATION_DESC {
	ui32 resolutionWidth;
	ui32 resolutionHeight;
	struct {
		bool enabled;
		ui32 resolution;
	} windowAttachment;

	SV::FrameBuffer* output = nullptr;
};

namespace SV {

	class Scene;

	class Renderer : public SV::EngineDevice {
		
		// 2D
		RenderQueue2D m_RenderQueue2D;
		Renderer2D m_Renderer2D;
		RenderLayer m_DefaultRenderLayer;

		std::vector<std::pair<TileMap*, XMMATRIX>> m_TileMaps;


		PostProcess m_PostProcess;

		FrameBuffer m_Offscreen;
		FrameBuffer* m_pOutput;

		SV::Camera* m_pCamera;
		XMMATRIX m_ViewProjectionMatrix;

		SV::uvec2 m_Resolution;
		bool m_WindowAttachment;
		ui32 m_WindowResolution;

		Renderer();
		~Renderer();

		bool Initialize(SV_RENDERER_INITIALIZATION_DESC& desc);
		bool Close();

		void BeginFrame();
		void Render();
		void EndFrame();

	public:
		friend Engine;
		friend Window;

		inline RenderQueue2D& GetRenderQueue2D() noexcept { return m_RenderQueue2D; }

		inline SV::Camera* GetCamera() const noexcept { return m_pCamera; };
		inline void SetCamera(SV::Camera* camera) noexcept { m_pCamera = camera; };

		inline void SetOutput(SV::FrameBuffer& output) noexcept { m_pOutput = &output; }

		void SetResolution(ui32 width, ui32 height);

		inline uvec2 GetResolution() const noexcept { return m_Resolution; }
		inline ui32 GetResolutionWidth() const noexcept { return m_Resolution.x; }
		inline ui32 GetResolutionHeight() const noexcept { return m_Resolution.y; }
		inline float GetResolutionAspect() const noexcept { return float(m_Resolution.x) / float(m_Resolution.y); }

		inline PostProcess& GetPostProcess() noexcept { return m_PostProcess; }

		void DrawScene(SV::Scene& scene);

	private:
		void UpdateResolution();

	};

}