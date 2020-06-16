#pragma once

#include "Graphics.h"

namespace SV {

	class PostProcess {
		Shader m_PPVertexShader;
		Shader m_DefaultPPPixelShader;
		VertexBuffer m_PPVertexBuffer;
		InputLayout m_PPInputLayout;
		Sampler m_PPSampler;

	public:
		bool Initialize(Graphics& gfx);
		bool Close(Graphics& gfx);

		void DefaultPP(FrameBuffer& input, FrameBuffer& output, Graphics& gfx, CommandList cmd);

	};

}