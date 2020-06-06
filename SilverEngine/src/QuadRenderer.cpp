#include "core.h"
#include "QuadRenderer.h"

namespace SV {

	bool QuadRenderer::Initialize(GraphicsDevice& device)
	{
		SV::vec2 vertices[] = {
			{-0.5,  0.5f},
			{ 0.5,  0.5f},
			{-0.5, -0.5f},
			{ 0.5, -0.5f}
		};
		
		device.ValidateVertexBuffer(&m_InstanceBuffer);

		device.ValidateVertexBuffer(&m_QuadVertexBuffer);
		device.ValidateShader(&m_QuadVertexShader);
		device.ValidateShader(&m_QuadPixelShader);
		device.ValidateInputLayout(&m_QuadInputLayout);

		if (!m_InstanceBuffer->Create(BATCH_COUNT * sizeof(SV::QuadInstance), SV_GFX_USAGE_DYNAMIC, true, false, nullptr, device)) {
			SV::LogE("Can't create Quad Instance buffer");
			return false;
		}

		if (!m_QuadVertexBuffer->Create(4 * sizeof(SV::vec2), SV_GFX_USAGE_STATIC, false, false, vertices, device)) {
			SV::LogE("Can't create Quad VertexBuffer");
			return false;
		}

		if (!m_QuadVertexShader->Create(SV_GFX_SHADER_TYPE_VERTEX, "QuadVertex.cso", device)) {
			SV::LogE("Quad VertexShader not found");
			return false;
		}
		if (!m_QuadPixelShader->Create(SV_GFX_SHADER_TYPE_PIXEL, "QuadPixel.cso", device)) {
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

		return true;
	}

	void QuadRenderer::Draw(std::vector<QuadInstance>& instances, CommandList& cmd)
	{
		ui32 count = ui32(instances.size());

		GraphicsDevice& device = cmd.GetDevice();

		device.SetTopology(SV_GFX_TOPOLOGY_TRIANGLESTRIP, cmd);

		m_QuadVertexShader->Bind(cmd);
		m_QuadPixelShader->Bind(cmd);
		m_QuadVertexBuffer->Bind(0u, sizeof(SV::vec2), 0u, cmd);
		m_QuadInputLayout->Bind(cmd);
		m_InstanceBuffer->Bind(1u, sizeof(SV::QuadInstance), 0u, cmd);
		
		for (ui32 i = 0; i < count;) {
			ui32 batch = count - i;
			if (batch > BATCH_COUNT) batch = BATCH_COUNT;

			m_InstanceBuffer->Update(instances.data() + i, batch * sizeof(QuadInstance), cmd);
			device.DrawInstanced(4, batch, 0u, 0u, cmd);

			i += batch;
		}

		m_InstanceBuffer->Unbind(cmd);
		m_QuadVertexShader->Unbind(cmd);
		m_QuadPixelShader->Unbind(cmd);
		m_QuadVertexBuffer->Unbind(cmd);
		m_QuadInputLayout->Unbind(cmd);
	}

	bool QuadRenderer::Close()
	{
		m_QuadVertexBuffer->Release();
		m_QuadVertexShader->Release();
		m_QuadPixelShader->Release();
		m_QuadInputLayout->Release();	
		m_InstanceBuffer->Release();
		return true;
	}

}