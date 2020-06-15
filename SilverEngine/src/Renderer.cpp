#include "core.h"

#include "Engine.h"

namespace SV {

	Renderer::Renderer() 
		: m_DefaultRenderLayer()
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

		if (!TileMap::CreateShaders(device)) {
			SV::LogE("Can't create TileMap shaders");
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

		m_ViewProjectionMatrix = m_pCamera->GetViewMatrix() * m_pCamera->GetProjectionMatrix();
		
		// 2D
		m_RenderQueue2D.Begin(m_pCamera);
		m_DefaultRenderLayer.Reset();
		m_TileMaps.clear();
	}
	void Renderer::Render()
	{
		if (m_pCamera == nullptr) {
			SV::LogW("Where are the camera ;)");
			return;
		}

		m_RenderQueue2D.AddLayer(m_DefaultRenderLayer);
		m_RenderQueue2D.End();

		Graphics& device = GetEngine().GetGraphics();
		CommandList cmd = device.BeginCommandList();

		// Clear buffers
		m_Offscreen->Clear(SVColor4f::BLACK, cmd);

		// Offscreen binding
		m_Offscreen->Bind(0u, cmd);
		device.SetViewport(0u, 0.f, 0.f, m_Offscreen->GetWidth(), m_Offscreen->GetHeight(), 0.f, 1.f, cmd);

		// Draw TileMaps
		if (m_TileMaps.size() > 0) {
			TileMap::BindShaders(cmd);
			for (ui32 i = 0; i < m_TileMaps.size(); ++i) {
				TileMap& tileMap = *m_TileMaps[i].first;
				XMMATRIX matrix = m_TileMaps[i].second;
				matrix = XMMatrixTranspose(matrix * m_ViewProjectionMatrix);
				
				tileMap.Bind(matrix, cmd);
				tileMap.Draw(cmd);
			}
			m_TileMaps.back().first->Unbind(cmd);
			TileMap::UnbindShaders(cmd);
		}

		// Draw 2D primitives
		m_Renderer2D.DrawRenderQueue(m_RenderQueue2D, cmd);

		// Offscreen to backBuffer
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
		{
			auto& layers = scene.GetComponentsList(SpriteLayerComponent::ID);

			ui32 layerSize = SpriteLayerComponent::SIZE;

			for (ui32 i = 0; i < layers.size(); i += layerSize) {
				SpriteLayerComponent* layerComp = reinterpret_cast<SpriteLayerComponent*>(&layers[i]);
				if (layerComp->defaultRendering) {
					layerComp->renderLayer->Reset();
					m_RenderQueue2D.AddLayer(*layerComp->renderLayer.get());
				}
			}
		}

		// Fill RenderQueues
		auto& sprites = scene.GetComponentsList(SpriteComponent::ID);

		RenderLayer* layer;
		ui32 CompSize = SpriteComponent::SIZE;

		for (ui32 i = 0; i < sprites.size(); i += CompSize) {
			SpriteComponent* sprComp = reinterpret_cast<SpriteComponent*>(&sprites[i]);

			// Get sprite layer
			layer = nullptr;
			if (sprComp->spriteLayer != SV_INVALID_ENTITY) {
				SpriteLayerComponent* layerComp = scene.GetComponent<SpriteLayerComponent>(sprComp->spriteLayer);
				if (layerComp) {
					if (!layerComp->defaultRendering) continue;
					layer = layerComp->renderLayer.get();
				}
			}
			if(!layer) layer = &m_DefaultRenderLayer;

			Transform& trans = scene.GetTransform(sprComp->entity);

			// Add instance to the queue
			if (sprComp->sprite.pTextureAtlas) {
				m_RenderQueue2D.DrawSprite(trans.GetWorldMatrix(), sprComp->color, sprComp->sprite, *layer);
			}
			else {
				m_RenderQueue2D.DrawQuad(trans.GetWorldMatrix(), sprComp->color, *layer);
			}
		}

		// Get tileMaps
		{
			auto& tileMaps = scene.GetComponentsList(TileMapComponent::ID);

			ui32 compSize = SV::TileMapComponent::SIZE;

			for (ui32 i = 0; i < tileMaps.size(); i += compSize) {
				SV::TileMapComponent* tileMapComp = reinterpret_cast<TileMapComponent*>(&tileMaps[i]);
				m_TileMaps.emplace_back(tileMapComp->tileMap.get(), tileMapComp->pScene->GetTransform(tileMapComp->entity).GetWorldMatrix());
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