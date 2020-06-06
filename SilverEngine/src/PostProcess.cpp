#include "core.h"

#include "PostProcess.h"

namespace SV {

	bool PostProcess::Initialize(GraphicsDevice& device)
	{
		device.ValidateShader(&m_PPVertexShader);
		device.ValidateShader(&m_DefaultPPPixelShader);
		device.ValidateVertexBuffer(&m_PPVertexBuffer);
		device.ValidateSampler(&m_PPSampler);
		device.ValidateInputLayout(&m_PPInputLayout);

		// Vertex Buffer
		{
			SV::vec2 coords[] = {
				{-1.0f,  1.0f},
				{ 1.0f,  1.0f},
				{-1.0f, -1.0f},
				{ 1.0f, -1.0f}
			};

			if (!m_PPVertexBuffer->Create(4 * sizeof(SV::vec2), SV_GFX_USAGE_STATIC, false, false, coords, device)) {
				SV::LogE("Can't create PostProcess Vertex Buffer");
				return false;
			}
		}

		// Shaders
		if (!m_PPVertexShader->Create(SV_GFX_SHADER_TYPE_VERTEX, "PPVertex.cso", device)) {
			SV::LogE("Post Process Vertex Shader not found");
			return false;
		}
		if (!m_DefaultPPPixelShader->Create(SV_GFX_SHADER_TYPE_PIXEL, "PPDefaultPixel.cso", device)) {
			SV::LogE("Post Process Pixel Shader not found");
			return false;
		}

		// Sampler
		if (!m_PPSampler->Create(SV_GFX_TEXTURE_ADDRESS_MIRROR, SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR, device)) {
			SV::LogE("Can't create PostProcess Sampler State");
			return false;
		}

		// InputLayout
		{
			SV_GFX_INPUT_ELEMENT_DESC desc[] = {
				{"Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 0u, false, 0u}
			};
			m_PPInputLayout->Create(desc, 1, m_PPVertexShader, device);
		}

		return true;
	}

	bool PostProcess::Close()
	{
		m_PPVertexShader->Release();
		m_DefaultPPPixelShader->Release();
		m_PPVertexBuffer->Release();
		m_PPInputLayout->Release();
		m_PPSampler->Release();
		return true;
	}

	void PostProcess::DefaultPP(FrameBuffer& input, FrameBuffer& output, CommandList& cmd)
	{
		GraphicsDevice& device = cmd.GetDevice();

		device.SetTopology(SV_GFX_TOPOLOGY_TRIANGLESTRIP, cmd);
		device.SetViewport(0u, 0.f, 0.f, output->GetWidth(), output->GetHeight(), 0.f, 1.f, cmd);

		m_PPVertexShader->Bind(cmd);
		m_DefaultPPPixelShader->Bind(cmd);
		m_PPInputLayout->Bind(cmd);
		m_PPVertexBuffer->Bind(0u, sizeof(SV::vec2), 0u, cmd);

		m_PPSampler->Bind(SV_GFX_SHADER_TYPE_PIXEL, 0u, cmd);
		input->BindAsTexture(SV_GFX_SHADER_TYPE_PIXEL, 0u, cmd);
		output->Bind(0u, cmd);

		device.Draw(4u, 0u, cmd);

		input->UnbindAsTexture(SV_GFX_SHADER_TYPE_PIXEL, 0u, cmd);
		output->Unbind(cmd);
		m_PPSampler->Unbind(SV_GFX_SHADER_TYPE_PIXEL, 0u, cmd);

		m_PPVertexBuffer->Unbind(cmd);
		m_PPInputLayout->Unbind(cmd);
		m_DefaultPPPixelShader->Unbind(cmd);
		m_PPVertexShader->Unbind(cmd);
	}

}