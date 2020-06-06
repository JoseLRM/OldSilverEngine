#pragma once

#include "core.h"

namespace SV {

	class QuadRenderer {
		VertexBuffer	m_QuadVertexBuffer;
		Shader			m_QuadVertexShader;
		Shader			m_QuadPixelShader;
		InputLayout		m_QuadInputLayout;

		const ui32 BATCH_COUNT = 1000u;
		VertexBuffer m_InstanceBuffer;

	public:
		bool Initialize(GraphicsDevice& device);
		bool Close();

		void Draw(std::vector<QuadInstance>& instances, CommandList& cmd);

	};

}