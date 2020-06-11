#include "core.h"

#include "Engine.h"
#include "SpriteComponent.h"

namespace SV {

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	bool Renderer::Initialize(SV_RENDERER_INITIALIZATION_DESC& desc)
	{
		SV::Graphics& device = GetEngine().GetGraphics();
		SV::Window& window = GetEngine().GetWindow();

		m_Resolution = SV::uvec2(desc.resolutionWidth, desc.resolutionHeight);
		if (desc.windowAttachment.enabled) {
			m_WindowAttachment = true;
			m_WindowResolution = desc.windowAttachment.resolution;
			UpdateResolution();
		}

		if (!m_PostProcess.Initialize(device)) {
			SV::LogE("Can't initialize Post Process");
			return false;
		}

		if (!m_Renderer2D.Initialize(device)) {
			SV::LogE("Can't initialize Renderer2D");
			return false;
		}

		device.ValidateFrameBuffer(&m_Offscreen);

		if (!m_Offscreen->Create(m_Resolution.x, m_Resolution.y, SV_GFX_FORMAT_R8G8B8A8_UNORM, true, device)) {
			SV::LogE("Can't create offscreen framebuffer");
			return false;
		}

		return true;
	}

	bool Renderer::Close()
	{
		if (!m_Renderer2D.Close()) {
			SV::LogE("Can't close Renderer2D");
			return false;
		}

		if (!m_PostProcess.Close()) {
			SV::LogE("Can't close Post Process");
			return false;
		}

		return true;
	}

	void Renderer::BeginFrame()
	{
		if (m_pCamera == nullptr) return;

		m_RenderQueue2D.Begin(m_pCamera);
	}
	void Renderer::Render()
	{
		if (m_pCamera == nullptr) {
			SV::LogW("Where are the camera ;)");
			return;
		}

		m_RenderQueue2D.End();

		Graphics& device = GetEngine().GetGraphics();

		CommandList cmd = device.BeginCommandList();

		m_Offscreen->Clear(SVColor4f::BLACK, cmd);

		m_Offscreen->Bind(0u, cmd);
		device.SetViewport(0u, 0.f, 0.f, m_Offscreen->GetWidth(), m_Offscreen->GetHeight(), 0.f, 1.f, cmd);

		for (ui32 i = 0; i < m_RenderQueue2D.m_pLayers.size(); ++i) {
			m_Renderer2D.DrawQuads(m_RenderQueue2D.m_pLayers[i]->quadInstances, cmd);
			m_Renderer2D.DrawSprites(m_RenderQueue2D.m_pLayers[i]->spriteInstances, cmd);
			m_Renderer2D.DrawEllipses(m_RenderQueue2D.m_pLayers[i]->ellipseInstances, cmd);
			m_Renderer2D.DrawPoints(m_RenderQueue2D.m_pLayers[i]->pointInstances, cmd);
			m_Renderer2D.DrawLines(m_RenderQueue2D.m_pLayers[i]->lineInstances, cmd);
		}

		m_PostProcess.DefaultPP(m_Offscreen, device.GetMainFrameBuffer(), cmd);
	}
	void Renderer::EndFrame()
	{
	}

	void Renderer::SetResolution(ui32 width, ui32 height)
	{
		if (m_WindowAttachment) {
			ui32 resolution = (height > width) ? height : width;
			m_WindowResolution = resolution;
		}
		else {
			if (m_Resolution.x == width && m_Resolution.y == height) return;
			m_Resolution = { width, height };
		}

		UpdateResolution();
	}

	void Renderer::DrawScene(SV::Scene& scene)
	{
		auto& sprites = scene.GetComponentsList(SpriteComponent::ID);

		ui32 CompSize = SpriteComponent::SIZE;
		for (ui32 i = 0; i < sprites.size(); i += CompSize) {
			SpriteComponent* sprComp = reinterpret_cast<SpriteComponent*>(&sprites[i]);

			if (sprComp->sprite.pTextureAtlas) {

			}
		}
	}

	void Renderer::UpdateResolution()
	{
		if (m_WindowAttachment) {
			SV::Window& window = GetEngine().GetWindow();

			float aspect = window.GetAspect();

			if (aspect >= 1.f) {
				m_Resolution.x = m_WindowResolution;
				m_Resolution.y = ui32(float(m_WindowResolution) / aspect);
			}
			else {
				m_Resolution.x = ui32(float(m_WindowResolution) * aspect);
				m_Resolution.y = m_WindowResolution;
			}
		}

		if(m_Offscreen.IsValid()) m_Offscreen->Resize(m_Resolution.x, m_Resolution.y, GetEngine().GetGraphics());
	}

}