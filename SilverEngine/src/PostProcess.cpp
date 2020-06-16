#include "core.h"

#include "PostProcess.h"

namespace SV {

	bool PostProcess::Initialize(Graphics& gfx)
	{
		// Vertex Buffer
		{
			SV::vec2 coords[] = {
				{-1.0f,  1.0f},
				{ 1.0f,  1.0f},
				{-1.0f, -1.0f},
				{ 1.0f, -1.0f}
			};

			if (!gfx.CreateVertexBuffer(4 * sizeof(SV::vec2), SV_GFX_USAGE_STATIC, SV_GFX_CPU_ACCESS_NONE, coords, m_PPVertexBuffer)) {
				SV::LogE("Can't create PostProcess Vertex Buffer");
				return false;
			}
		}

		// Shaders
		if (!gfx.CreateShader(SV_GFX_SHADER_TYPE_VERTEX, "shaders/PPVertex.cso", m_PPVertexShader)) {
			SV::LogE("Post Process Vertex Shader not found");
			return false;
		}
		if (!gfx.CreateShader(SV_GFX_SHADER_TYPE_PIXEL, "shaders/PPDefaultPixel.cso", m_DefaultPPPixelShader)) {
			SV::LogE("Post Process Pixel Shader not found");
			return false;
		}

		// Sampler
		if (!gfx.CreateSampler(SV_GFX_TEXTURE_ADDRESS_MIRROR, SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR, m_PPSampler)) {
			SV::LogE("Can't create PostProcess Sampler State");
			return false;
		}

		// InputLayout
		{
			SV_GFX_INPUT_ELEMENT_DESC desc[] = {
				{"Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 0u, false, 0u}
			};
			gfx.CreateInputLayout(desc, 1, m_PPVertexShader, m_PPInputLayout);
		}

		return true;
	}

	bool PostProcess::Close(Graphics& gfx)
	{
		gfx.ReleaseVertexBuffer(m_PPVertexBuffer);
		gfx.ReleaseShader(m_PPVertexShader);
		gfx.ReleaseShader(m_DefaultPPPixelShader);
		gfx.ReleaseInputLayout(m_PPInputLayout);
		gfx.ReleaseSampler(m_PPSampler);
		return true;
	}

	void PostProcess::DefaultPP(FrameBuffer& input, FrameBuffer& output, Graphics& gfx, CommandList cmd)
	{
		gfx.SetTopology(SV_GFX_TOPOLOGY_TRIANGLESTRIP, cmd);
		gfx.SetViewport(0u, 0.f, 0.f, output->GetWidth(), output->GetHeight(), 0.f, 1.f, cmd);

		gfx.BindVertexBuffer(0u, sizeof(SV::vec2), 0u, m_PPVertexBuffer, cmd);
		gfx.BindShader(m_PPVertexShader, cmd);
		gfx.BindShader(m_DefaultPPPixelShader, cmd);
		gfx.BindInputLayout(m_PPInputLayout, cmd);

		gfx.BindSampler(0u, SV_GFX_SHADER_TYPE_PIXEL, m_PPSampler, cmd);
		gfx.BindFrameBuffer(output, cmd);
		gfx.BindFrameBufferAsTexture(0u, SV_GFX_SHADER_TYPE_PIXEL, input, cmd);

		gfx.Draw(4u, 0u, cmd);

		gfx.UnbindVertexBuffer(0u, cmd);
		gfx.UnbindShader(SV_GFX_SHADER_TYPE_VERTEX, cmd);
		gfx.UnbindShader(SV_GFX_SHADER_TYPE_PIXEL, cmd);
		gfx.UnbindInputLayout(cmd);

		gfx.UnbindSampler(0u, SV_GFX_SHADER_TYPE_PIXEL, cmd);
		gfx.UnbindTexture(0u, SV_GFX_SHADER_TYPE_PIXEL, cmd);
		gfx.UnbindFrameBuffers(cmd);
	}

}