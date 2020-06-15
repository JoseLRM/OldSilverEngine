#pragma once

#include "core.h"
#include "RenderQueue2D.h"
#include "Graphics.h"

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

		BlendState		m_OpaqueBlendState;
		BlendState		m_TransparentBlendState;

		VertexBuffer m_InstanceBuffer;

		struct GPUSpriteInstance {
			XMMATRIX tm;
			SV::vec4 texCoord;
			SV::Color4b color;
		};
		GPUSpriteInstance m_SpriteInstanceData[SV_GFX_QUAD_BATCH_COUNT];

	public:
		bool Initialize(Graphics& device);
		bool Close();

		void DrawRenderQueue(RenderQueue2D& rq, CommandList& cmd);

		void DrawQuads(std::vector<QuadInstance>& instances, bool transparent, CommandList& cmd);
		void DrawEllipses(std::vector<QuadInstance>& instances, bool transparent, CommandList& cmd);
		void DrawSprites(std::vector<SpriteInstance>& instances, bool transparent, CommandList& cmd);
		void DrawPoints(std::vector<PointInstance>& instances, bool transparent, CommandList& cmd);
		void DrawLines(std::vector<PointInstance>& instances, bool transparent, CommandList& cmd);

	private:
		void DrawQuadsOrEllipses(std::vector<QuadInstance>& instances, bool transparent, bool quad, CommandList& cmd);
		void DrawPointsOrLines(std::vector<PointInstance>& instances, bool transparent, bool point, CommandList& cmd);

		void BindBlendState(bool transparent, CommandList& cmd);
		void UnbindBlendState(bool transparent, CommandList& cmd);

	};

}