#include "core.h"

#include "Renderer2D.h"

namespace SV {

	bool Renderer2D::Initialize(Graphics& device)
	{
		SV::vec2 vertices[] = {
			{-0.5,  0.5f},
			{ 0.5,  0.5f},
			{-0.5, -0.5f},
			{ 0.5, -0.5f}
		};
		
		device.ValidateVertexBuffer(&m_InstanceBuffer);
		device.ValidateVertexBuffer(&m_VertexBuffer);
		
		device.ValidateShader(&m_QuadVertexShader);
		device.ValidateShader(&m_QuadPixelShader);
		device.ValidateInputLayout(&m_QuadInputLayout);
		
		device.ValidateShader(&m_SpriteVertexShader);
		device.ValidateShader(&m_SpritePixelShader);
		device.ValidateInputLayout(&m_SpriteInputLayout);
		
		device.ValidateShader(&m_EllipseVertexShader);
		device.ValidateShader(&m_EllipsePixelShader);

		device.ValidateShader(&m_PointVertexShader);
		device.ValidateShader(&m_PointPixelShader);
		device.ValidateInputLayout(&m_PointInputLayout);

		if (!m_InstanceBuffer->Create(SV_GFX_QUAD_BATCH_COUNT * sizeof(GPUSpriteInstance), SV_GFX_USAGE_DYNAMIC, true, false, nullptr, device)) {
			SV::LogE("Can't create Quad Instance buffer");
			return false;
		}

		if (!m_VertexBuffer->Create(4 * sizeof(SV::vec2), SV_GFX_USAGE_STATIC, false, false, vertices, device)) {
			SV::LogE("Can't create Quad VertexBuffer");
			return false;
		}

		// QUAD RENDERING
		if (!m_QuadVertexShader->Create(SV_GFX_SHADER_TYPE_VERTEX, "shaders/QuadVertex.cso", device)) {
			SV::LogE("Quad VertexShader not found");
			return false;
		}
		if (!m_QuadPixelShader->Create(SV_GFX_SHADER_TYPE_PIXEL, "shaders/QuadPixel.cso", device)) {
			SV::LogE("Quad PixelShader not found");
			return false;
		}

		{
			const SV_GFX_INPUT_ELEMENT_DESC inputs[] = {
				{"Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 0u, false, 0u},
				{"TM", 0u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 0u * sizeof(float), true, 1u},
				{"TM", 1u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 4u * sizeof(float), true, 1u},
				{"TM", 2u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 8u * sizeof(float), true, 1u},
				{"TM", 3u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 12u * sizeof(float), true, 1u},
				{"Color", 0u, SV_GFX_FORMAT_R8G8B8A8_UNORM, 1u, 16u * sizeof(float), true, 1u}
			};
			if (!m_QuadInputLayout->Create(inputs, 6, m_QuadVertexShader, device)) {
				SV::LogE("Can't create Quad InputLayout");
				return false;
			}
		}

		// SPRITE RENDERING
		if (!m_SpriteVertexShader->Create(SV_GFX_SHADER_TYPE_VERTEX, "shaders/SpriteVertex.cso", device)) {
			SV::LogE("Sprite VertexShader not found");
			return false;
		}
		if (!m_SpritePixelShader->Create(SV_GFX_SHADER_TYPE_PIXEL, "shaders/SpritePixel.cso", device)) {
			SV::LogE("Sprite PixelShader not found");
			return false;
		}

		{
			const SV_GFX_INPUT_ELEMENT_DESC inputs[] = {
				{"Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 0u, false, 0u},
				{"TM", 0u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 0u * sizeof(float), true, 1u},
				{"TM", 1u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 4u * sizeof(float), true, 1u},
				{"TM", 2u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 8u * sizeof(float), true, 1u},
				{"TM", 3u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 12u * sizeof(float), true, 1u},
				{"TexCoord", 0u, SV_GFX_FORMAT_R32G32B32A32_FLOAT, 1u, 16u * sizeof(float), true, 1u},
				{"Color", 0u, SV_GFX_FORMAT_R8G8B8A8_UNORM, 1u, 20u * sizeof(float), true, 1u}
			};
			if (!m_SpriteInputLayout->Create(inputs, 7, m_SpriteVertexShader, device)) {
				SV::LogE("Can't create Sprite InputLayout");
				return false;
			}
		}

		// ELLIPSE RENDERING
		if (!m_EllipseVertexShader->Create(SV_GFX_SHADER_TYPE_VERTEX, "shaders/EllipseVertex.cso", device)) {
			SV::LogE("Ellipse VertexShader not found");
			return false;
		}
		if (!m_EllipsePixelShader->Create(SV_GFX_SHADER_TYPE_PIXEL, "shaders/EllipsePixel.cso", device)) {
			SV::LogE("Ellipse PixelShader not found");
			return false;
		}

		// POINT RENDERING
		if (!m_PointVertexShader->Create(SV_GFX_SHADER_TYPE_VERTEX, "shaders/PointVertex.cso", device)) {
			SV::LogE("Point VertexShader not found");
			return false;
		}
		if (!m_PointPixelShader->Create(SV_GFX_SHADER_TYPE_PIXEL, "shaders/PointPixel.cso", device)) {
			SV::LogE("Point PixelShader not found");
			return false;
		}

		{
			const SV_GFX_INPUT_ELEMENT_DESC inputs[] = {
				{"Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 0u * sizeof(float), true, 1u},
				{"Color", 0u, SV_GFX_FORMAT_R8G8B8A8_UNORM, 0u, 2.f * sizeof(float), true, 1u}
			};
			if (!m_PointInputLayout->Create(inputs, 2, m_PointVertexShader, device)) {
				SV::LogE("Can't create Point InputLayout");
				return false;
			}
		}

		return true;
	}

	void Renderer2D::DrawRenderQueue(RenderQueue2D& rq, CommandList& cmd)
	{
		for (ui32 i = 0; i < rq.m_pLayers.size(); ++i) {
			DrawQuads(rq.m_pLayers[i]->quadInstances, cmd);
			DrawSprites(rq.m_pLayers[i]->spriteInstances, cmd);
			DrawEllipses(rq.m_pLayers[i]->ellipseInstances, cmd);
			DrawPoints(rq.m_pLayers[i]->pointInstances, cmd);
			DrawLines(rq.m_pLayers[i]->lineInstances, cmd);
		}
	}

	void Renderer2D::DrawQuads(std::vector<QuadInstance>& instances, CommandList& cmd)
	{
		DrawQuadsOrEllipses(instances, true, cmd);
	}
	void Renderer2D::DrawEllipses(std::vector<QuadInstance>& instances, CommandList& cmd)
	{
		DrawQuadsOrEllipses(instances, false, cmd);
	}

	void Renderer2D::DrawPoints(std::vector<PointInstance>& instances, CommandList& cmd)
	{
		DrawPointsOrLines(instances, true, cmd);
	}
	void Renderer2D::DrawLines(std::vector<PointInstance>& instances, CommandList& cmd)
	{
		DrawPointsOrLines(instances, false, cmd);
	}

	void Renderer2D::DrawSprites(std::vector<SpriteInstance>& instances, CommandList& cmd) 
	{
		ui32 count = ui32(instances.size());
		if (count == 0) return;

		Graphics& device = cmd.GetDevice();

		device.SetTopology(SV_GFX_TOPOLOGY_TRIANGLESTRIP, cmd);

		m_SpriteVertexShader->Bind(cmd);
		m_SpritePixelShader->Bind(cmd);
		m_VertexBuffer->Bind(0u, sizeof(SV::vec2), 0u, cmd);
		m_SpriteInputLayout->Bind(cmd);
		m_InstanceBuffer->Bind(1u, sizeof(GPUSpriteInstance), 0u, cmd);

		for (ui32 i = 0; i < count;) {
			ui32 batch = count - i;
			if (batch > SV_GFX_QUAD_BATCH_COUNT) batch = SV_GFX_QUAD_BATCH_COUNT;

			ui32 lastInstance = i + batch;

			// Update buffer
			for (ui32 j = i; j < lastInstance; ++j) {
				auto& instance = instances[j];
				ui32 spriteID = instance.sprite.ID;
				m_SpriteInstanceData[j - i] = { instance.tm, instance.sprite.pTextureAtlas->GetSpriteTexCoords(spriteID), instance.color };
			}
			m_InstanceBuffer->Update(m_SpriteInstanceData, batch * sizeof(GPUSpriteInstance), cmd);

			// Draw calls
			ui32 j = 0;
			while (j < batch) {
				ui32 instancesCount = batch;

				TextureAtlas* texAtlas = instances[i].sprite.pTextureAtlas;

				for (ui32 w = i + j + 1; w < lastInstance; ++w) {
					if (instances[w].sprite.pTextureAtlas != texAtlas) {
						instancesCount = w - j;
						break;
					}
				}

				texAtlas->Bind(SV_GFX_SHADER_TYPE_PIXEL, 0u, cmd);
				device.DrawInstanced(4, instancesCount, 0u, j, cmd);
				texAtlas->Unbind(SV_GFX_SHADER_TYPE_PIXEL, 0u, cmd);

				j += instancesCount;
			}

			i += batch;
		}

		m_InstanceBuffer->Unbind(cmd);
		m_SpriteVertexShader->Unbind(cmd);
		m_SpritePixelShader->Unbind(cmd);
		m_VertexBuffer->Unbind(cmd);
		m_SpriteInputLayout->Unbind(cmd);
	}

	void Renderer2D::DrawQuadsOrEllipses(std::vector<QuadInstance>& instances, bool quad, CommandList& cmd)
	{
		ui32 count = ui32(instances.size());
		if (count == 0) return;

		Graphics& device = cmd.GetDevice();

		// Binding
		device.SetTopology(SV_GFX_TOPOLOGY_TRIANGLESTRIP, cmd);

		if (quad) {
			m_QuadVertexShader->Bind(cmd);
			m_QuadPixelShader->Bind(cmd);
		}
		else {
			m_EllipseVertexShader->Bind(cmd);
			m_EllipsePixelShader->Bind(cmd);
		}

		m_VertexBuffer->Bind(0u, sizeof(SV::vec2), 0u, cmd);
		m_QuadInputLayout->Bind(cmd);
		m_InstanceBuffer->Bind(1u, sizeof(SV::QuadInstance), 0u, cmd);

		// Draw
		for (ui32 i = 0; i < count;) {
			ui32 batch = count - i;
			if (batch > SV_GFX_QUAD_BATCH_COUNT) batch = SV_GFX_QUAD_BATCH_COUNT;

			m_InstanceBuffer->Update(instances.data() + i, batch * sizeof(QuadInstance), cmd);
			device.DrawInstanced(4, batch, 0u, 0u, cmd);

			i += batch;
		}

		// Unbind
		m_InstanceBuffer->Unbind(cmd);
		m_VertexBuffer->Unbind(cmd);
		m_QuadInputLayout->Unbind(cmd);

		if (quad) {
			m_QuadVertexShader->Unbind(cmd);
			m_QuadPixelShader->Unbind(cmd);
		}
		else {
			m_EllipseVertexShader->Unbind(cmd);
			m_EllipsePixelShader->Unbind(cmd);
		}
	}

	void Renderer2D::DrawPointsOrLines(std::vector<PointInstance>& instances, bool point, CommandList& cmd)
	{
		ui32 count = ui32(instances.size());
		if (count == 0) return;

		Graphics& device = cmd.GetDevice();

		// Binding
		if(point) device.SetTopology(SV_GFX_TOPOLOGY_POINTS, cmd);
		else device.SetTopology(SV_GFX_TOPOLOGY_LINES, cmd);

		m_PointVertexShader->Bind(cmd);
		m_PointPixelShader->Bind(cmd);

		m_PointInputLayout->Bind(cmd);
		m_InstanceBuffer->Bind(0u, sizeof(SV::PointInstance), 0u, cmd);

		// Draw
		for (ui32 i = 0; i < count;) {
			ui32 batch = count - i;
			if (batch > SV_GFX_QUAD_BATCH_COUNT * 2u) batch = SV_GFX_QUAD_BATCH_COUNT * 2u;

			m_InstanceBuffer->Update(instances.data() + i, batch * sizeof(PointInstance), cmd);
			
			if(point) device.DrawInstanced(1, batch, 0u, 0u, cmd);
			else device.DrawInstanced(2, batch, 0u, 0u, cmd);

			i += batch;
		}

		// Unbind
		m_InstanceBuffer->Unbind(cmd);
		m_PointInputLayout->Unbind(cmd);

		m_PointVertexShader->Unbind(cmd);
		m_PointPixelShader->Unbind(cmd);
	}

	bool Renderer2D::Close()
	{
		m_VertexBuffer->Release();
		m_InstanceBuffer->Release();

		m_QuadVertexShader->Release();
		m_QuadPixelShader->Release();
		m_QuadInputLayout->Release();	
		
		m_SpriteVertexShader->Release();
		m_SpritePixelShader->Release();
		m_SpriteInputLayout->Release();	

		m_EllipseVertexShader->Release();
		m_EllipsePixelShader->Release();

		m_PointVertexShader->Release();
		m_PointPixelShader->Release();
		m_PointInputLayout->Release();
		
		return true;
	}

}
