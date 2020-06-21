#include "core.h"

#include "Renderer2D.h"

namespace SV {

	bool Renderer2D::Initialize(Graphics& graphics)
	{
		SV::vec2 vertices[] = {
			{-0.5,  0.5f},
			{ 0.5,  0.5f},
			{-0.5, -0.5f},
			{ 0.5, -0.5f}
		};

		if (!graphics.CreateVertexBuffer(SV_GFX_QUAD_BATCH_COUNT * sizeof(GPUSpriteInstance), SV_GFX_USAGE_DYNAMIC, SV_GFX_CPU_ACCESS_WRITE, nullptr, m_InstanceBuffer)) {
			SV::LogE("Can't create Quad Instance buffer");
			return false;
		}

		if (!graphics.CreateVertexBuffer(4 * sizeof(SV::vec2), SV_GFX_USAGE_STATIC, SV_GFX_CPU_ACCESS_NONE, vertices, m_VertexBuffer)) {
			SV::LogE("Can't create Quad VertexBuffer");
			return false;
		}

		// QUAD RENDERING
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_VERTEX, "shaders/QuadVertex.cso", m_QuadVertexShader)) {
			SV::LogE("Quad VertexShader not found");
			return false;
		}
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_PIXEL, "shaders/QuadPixel.cso", m_QuadPixelShader)) {
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
			if (!graphics.CreateInputLayout(inputs, 6, m_QuadVertexShader, m_QuadInputLayout)) {
				SV::LogE("Can't create Quad InputLayout");
				return false;
			}
		}

		// SPRITE RENDERING
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_VERTEX, "shaders/SpriteVertex.cso", m_SpriteVertexShader)) {
			SV::LogE("Sprite VertexShader not found");
			return false;
		}
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_PIXEL, "shaders/SpritePixel.cso", m_SpritePixelShader)) {
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
			if (!graphics.CreateInputLayout(inputs, 7, m_SpriteVertexShader, m_SpriteInputLayout)) {
				SV::LogE("Can't create Sprite InputLayout");
				return false;
			}
		}

		// ELLIPSE RENDERING
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_VERTEX, "shaders/EllipseVertex.cso", m_EllipseVertexShader)) {
			SV::LogE("Ellipse VertexShader not found");
			return false;
		}
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_PIXEL, "shaders/EllipsePixel.cso", m_EllipsePixelShader)) {
			SV::LogE("Ellipse PixelShader not found");
			return false;
		}

		// POINT RENDERING
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_VERTEX, "shaders/PointVertex.cso", m_PointVertexShader)) {
			SV::LogE("Point VertexShader not found");
			return false;
		}
		if (!graphics.CreateShader(SV_GFX_SHADER_TYPE_PIXEL, "shaders/PointPixel.cso", m_PointPixelShader)) {
			SV::LogE("Point PixelShader not found");
			return false;
		}

		{
			const SV_GFX_INPUT_ELEMENT_DESC inputs[] = {
				{"Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT, 0u, 0u * sizeof(float), true, 1u},
				{"Color", 0u, SV_GFX_FORMAT_R8G8B8A8_UNORM, 0u, 2.f * sizeof(float), true, 1u}
			};
			if (!graphics.CreateInputLayout(inputs, 2, m_PointVertexShader, m_PointInputLayout)) {
				SV::LogE("Can't create Point InputLayout");
				return false;
			}
		}

		// BLEND STATE
		{
			SV_GFX_BLEND_STATE_DESC blendDesc;
			blendDesc.SetDefault();

			blendDesc.independentRenderTarget = true;
			blendDesc.renderTargetDesc[0].enabled = true;
			blendDesc.renderTargetDesc[0].src = SV_GFX_BLEND_SRC_ALPHA;
			blendDesc.renderTargetDesc[0].srcAlpha = SV_GFX_BLEND_ONE;
			blendDesc.renderTargetDesc[0].dest = SV_GFX_BLEND_INV_SRC_ALPHA;
			blendDesc.renderTargetDesc[0].destAlpha = SV_GFX_BLEND_ONE;
			blendDesc.renderTargetDesc[0].op = SV_GFX_BLEND_OP_ADD;
			blendDesc.renderTargetDesc[0].opAlpha = SV_GFX_BLEND_OP_ADD;
			blendDesc.renderTargetDesc[0].writeMask = SV_GFX_COLOR_WRITE_ENABLE_ALL;

			if (!graphics.CreateBlendState(&blendDesc, m_TransparentBlendState)) {
				SV::LogE("Can't Create 2D Transparent Blend State");
				return false;
			}
		}

		// DEPTH STENCIL STATE
		{
			SV_GFX_DEPTH_STENCIL_STATE_DESC desc;
			desc.SetDefault();
			desc.stencilEnable = true;
			desc.stencilReadMask = 0xFF;
			desc.stencilWriteMask = 0xFF;
			desc.frontFaceOp.stencilDepthFailOp = SV_GFX_STENCIL_OP_KEEP;
			desc.frontFaceOp.stencilFailOp = SV_GFX_STENCIL_OP_KEEP;
			desc.frontFaceOp.stencilPassOp = SV_GFX_STENCIL_OP_REPLACE;
			desc.frontFaceOp.stencilFunction = SV_GFX_COMPARISON_NOT_EQUAL;

			desc.backFaceOp = desc.frontFaceOp;

			if (!graphics.CreateDepthStencilState(&desc, m_OpaqueDepthStencilState))
			{
				SV::LogE("Can' create 2D Opaque DepthStencil State");
				return false;
			}
		}

		return true;
	}

	void Renderer2D::DrawRenderQueue(RenderQueue2D& rq, Graphics& gfx, CommandList cmd)
	{
		for (ui32 i = 0; i < rq.m_pLayers.size(); ++i) {
			RenderLayer& layer = *rq.m_pLayers[i];
			DrawQuads(layer.quadInstances, layer.IsTransparent(), gfx, cmd);
			DrawSprites(layer.spriteInstances, layer.IsTransparent(), gfx, cmd);
			DrawEllipses(layer.ellipseInstances, layer.IsTransparent(), gfx, cmd);
			DrawPoints(layer.pointInstances, layer.IsTransparent(), gfx, cmd);
			DrawLines(layer.lineInstances, layer.IsTransparent(), gfx, cmd);
		}
	}

	void Renderer2D::DrawQuads(std::vector<QuadInstance>& instances, bool transparent, Graphics& gfx, CommandList cmd)
	{
		DrawQuadsOrEllipses(instances, transparent, true, gfx, cmd);
	}
	void Renderer2D::DrawEllipses(std::vector<QuadInstance>& instances, bool transparent, Graphics& gfx, CommandList cmd)
	{
		DrawQuadsOrEllipses(instances, transparent, false, gfx, cmd);
	}

	void Renderer2D::DrawPoints(std::vector<PointInstance>& instances, bool transparent, Graphics& gfx, CommandList cmd)
	{
		DrawPointsOrLines(instances, transparent, true, gfx, cmd);
	}
	void Renderer2D::DrawLines(std::vector<PointInstance>& instances, bool transparent, Graphics& gfx, CommandList cmd)
	{
		DrawPointsOrLines(instances, transparent, false, gfx, cmd);
	}

	void Renderer2D::DrawSprites(std::vector<SpriteInstance>& instances, bool transparent, Graphics& gfx, CommandList cmd)
	{
		ui32 count = ui32(instances.size());
		if (count == 0) return;

		gfx.SetTopology(SV_GFX_TOPOLOGY_TRIANGLESTRIP, cmd);

		BindState(transparent, gfx, cmd);

		gfx.BindShader(m_SpriteVertexShader, cmd);
		gfx.BindShader(m_SpritePixelShader, cmd);
		gfx.BindInputLayout(m_SpriteInputLayout, cmd);

		{
			ui32 strides[] = {
				sizeof(SV::vec2), sizeof(GPUSpriteInstance)
			};
			ui32 offsets[] = {
				0u, 0u
			};
			VertexBuffer* vertexBuffers[] = {
				&m_VertexBuffer, &m_InstanceBuffer
			};
			gfx.BindVertexBuffers(0u, 2u, strides, offsets, vertexBuffers, cmd);
		}

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

			gfx.UpdateVertexBuffer(m_SpriteInstanceData, batch * sizeof(GPUSpriteInstance), m_InstanceBuffer, cmd);

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

				gfx.BindTexture(0u, SV_GFX_SHADER_TYPE_PIXEL, texAtlas->GetTexture(), cmd);
				gfx.BindSampler(0u, SV_GFX_SHADER_TYPE_PIXEL, texAtlas->GetSampler(), cmd);
				gfx.DrawInstanced(4, instancesCount, 0u, j, cmd);

				j += instancesCount;
			}

			i += batch;
		}

		gfx.UnbindTextures(cmd);
		gfx.UnbindSamplers(cmd);

		gfx.UnbindShader(SV_GFX_SHADER_TYPE_VERTEX, cmd);
		gfx.UnbindShader(SV_GFX_SHADER_TYPE_PIXEL, cmd);
		gfx.UnbindInputLayout(cmd);
		gfx.UnbindVertexBuffers(cmd);

		UnbindBlendState(transparent, gfx, cmd);
	}

	void Renderer2D::DrawQuadsOrEllipses(std::vector<QuadInstance>& instances, bool transparent, bool quad, Graphics& gfx, CommandList cmd)
	{
		ui32 count = ui32(instances.size());
		if (count == 0) return;

		// Binding
		gfx.SetTopology(SV_GFX_TOPOLOGY_TRIANGLESTRIP, cmd);

		BindState(transparent, gfx, cmd);

		if (quad) {
			gfx.BindShader(m_QuadVertexShader, cmd);
			gfx.BindShader(m_QuadPixelShader, cmd);
		}
		else {
			gfx.BindShader(m_EllipseVertexShader, cmd);
			gfx.BindShader(m_EllipsePixelShader, cmd);
		}

		gfx.BindInputLayout(m_QuadInputLayout, cmd);

		{
			ui32 strides[] = {
				sizeof(SV::vec2), sizeof(SV::QuadInstance)
			};
			ui32 offsets[] = {
				0u, 0u
			};
			VertexBuffer* vertexBuffers[] = {
				&m_VertexBuffer, & m_InstanceBuffer
			};

			gfx.BindVertexBuffers(0u, 2u, strides, offsets, vertexBuffers, cmd);
		}

		// Draw
		for (ui32 i = 0; i < count;) {
			ui32 batch = count - i;
			if (batch > SV_GFX_QUAD_BATCH_COUNT) batch = SV_GFX_QUAD_BATCH_COUNT;

			gfx.UpdateVertexBuffer(instances.data() + i, batch * sizeof(QuadInstance), m_InstanceBuffer, cmd);
			gfx.DrawInstanced(4, batch, 0u, 0u, cmd);

			i += batch;
		}

		// Unbind
		gfx.UnbindShader(SV_GFX_SHADER_TYPE_VERTEX, cmd);
		gfx.UnbindShader(SV_GFX_SHADER_TYPE_PIXEL, cmd);

		gfx.UnbindInputLayout(cmd);
		gfx.UnbindVertexBuffers(cmd);

		UnbindBlendState(transparent, gfx, cmd);
	}

	void Renderer2D::DrawPointsOrLines(std::vector<PointInstance>& instances, bool transparent, bool point, Graphics& gfx, CommandList cmd)
	{
		ui32 count = ui32(instances.size());
		if (count == 0) return;

		// Binding
		if(point) gfx.SetTopology(SV_GFX_TOPOLOGY_POINTS, cmd);
		else gfx.SetTopology(SV_GFX_TOPOLOGY_LINES, cmd);

		BindState(transparent, gfx, cmd);

		gfx.BindShader(m_PointVertexShader, cmd);
		gfx.BindShader(m_PointPixelShader, cmd);

		gfx.BindInputLayout(m_PointInputLayout, cmd);
		gfx.BindVertexBuffer(0u, sizeof(SV::PointInstance), 0u, m_InstanceBuffer, cmd);

		// Draw
		for (ui32 i = 0; i < count;) {
			ui32 batch = count - i;
			if (batch > SV_GFX_QUAD_BATCH_COUNT * 2u) batch = SV_GFX_QUAD_BATCH_COUNT * 2u;

			gfx.UpdateVertexBuffer(instances.data() + i, batch * sizeof(PointInstance), m_InstanceBuffer, cmd);
			
			if(point) gfx.DrawInstanced(1, batch, 0u, 0u, cmd);
			else gfx.DrawInstanced(2, batch, 0u, 0u, cmd);

			i += batch;
		}

		// Unbind
		gfx.UnbindShader(SV_GFX_SHADER_TYPE_VERTEX, cmd);
		gfx.UnbindShader(SV_GFX_SHADER_TYPE_PIXEL, cmd);

		gfx.UnbindInputLayout(cmd);
		gfx.UnbindVertexBuffer(0u, cmd);

		UnbindBlendState(transparent, gfx, cmd);
	}

	void Renderer2D::BindState(bool transparent, Graphics& gfx, CommandList cmd)
	{
		if (transparent) gfx.BindBlendState(m_TransparentBlendState, cmd);
		else gfx.BindDepthStencilState(1u, m_OpaqueDepthStencilState, cmd);
	}
	void Renderer2D::UnbindBlendState(bool transparent, Graphics& gfx, CommandList cmd)
	{
		if (transparent) gfx.UnbindBlendState(cmd);
		else gfx.UnbindDepthStencilState(cmd);
	}

	bool Renderer2D::Close(Graphics& gfx)
	{
		gfx.ReleaseVertexBuffer(m_VertexBuffer);
		gfx.ReleaseVertexBuffer(m_InstanceBuffer);

		gfx.ReleaseShader(m_QuadVertexShader);
		gfx.ReleaseShader(m_QuadPixelShader);
		gfx.ReleaseInputLayout(m_QuadInputLayout);
		
		gfx.ReleaseShader(m_SpriteVertexShader);
		gfx.ReleaseShader(m_SpritePixelShader);
		gfx.ReleaseInputLayout(m_SpriteInputLayout);

		gfx.ReleaseShader(m_EllipseVertexShader);
		gfx.ReleaseShader(m_EllipsePixelShader);

		gfx.ReleaseShader(m_PointVertexShader);
		gfx.ReleaseShader(m_PointPixelShader);
		gfx.ReleaseInputLayout(m_PointInputLayout);
		
		gfx.ReleaseBlendState(m_TransparentBlendState);
		
		return true;
	}

}
