#pragma once

#include "core.h"

namespace SV {

	class Renderer2D {
		VertexBuffer	m_VertexBuffer;

		Shader			m_QuadVertexShader;
		Shader			m_QuadPixelShader;
		InputLayout		m_QuadInputLayout;

		Shader			m_EllipseVertexShader;
		Shader			m_EllipsePixelShader;

		Shader			m_SpriteVertexShader;
		Shader			m_SpritePixelShader;
		InputLayout		m_SpriteInputLayout;

		Shader			m_PointVertexShader;
		Shader			m_PointPixelShader;
		InputLayout		m_PointInputLayout;

		VertexBuffer m_InstanceBuffer;

		struct GPUSpriteInstance {
			XMMATRIX tm;
			SV::vec4 texCoord;
			SV::Color4b color;
		};
		GPUSpriteInstance m_SpriteInstanceData[SV_GFX_QUAD_BATCH_COUNT];

	public:
		bool Initialize(GraphicsDevice& device);
		bool Close();

		void DrawQuads(std::vector<QuadInstance>& instances, CommandList& cmd);
		void DrawEllipses(std::vector<QuadInstance>& instances, CommandList& cmd);
		void DrawSprites(std::vector<SpriteInstance>& instances, CommandList& cmd);
		void DrawPoints(std::vector<PointInstance>& instances, CommandList& cmd);
		void DrawLines(std::vector<PointInstance>& instances, CommandList& cmd);

	private:
		void DrawQuadsOrEllipses(std::vector<QuadInstance>& instances, bool quad, CommandList& cmd);
		void DrawPointsOrLines(std::vector<PointInstance>& instances, bool point, CommandList& cmd);

	};

}