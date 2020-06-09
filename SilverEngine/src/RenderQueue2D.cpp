#include "core.h"

#include "RenderQueue2D.h"

namespace SV {

	inline XMMATRIX CreateMatrix(const vec2& pos, const vec2& size, float rotation, const XMMATRIX& vpm)
	{
		return XMMatrixTranspose(XMMatrixScaling(size.x, size.y, 0.f) * XMMatrixRotationZ(rotation)
			* XMMatrixTranslation(pos.x, pos.y, 0.f) * vpm);
	}
	inline XMMATRIX CreateMatrix(const vec2& pos, const vec2& size, const XMMATRIX& vpm)
	{
		return XMMatrixTranspose(XMMatrixScaling(size.x, size.y, 0.f) * XMMatrixTranslation(pos.x, pos.y, 0.f) * vpm);
	}
	inline SV::vec2 CalculateScreenSpacePos(const vec2& pos, const XMMATRIX& vpm)
	{
		XMVECTOR position = XMVectorSet(pos.x, pos.y, 0.f, 1.f);
		position = XMVector3Transform(position, vpm);

		return SV::vec2(XMVectorGetX(position), XMVectorGetY(position));
	}

	void RenderLayer::Reset() noexcept
	{
		spriteInstances.clear();
		quadInstances.clear();
		ellipseInstances.clear();
		lineInstances.clear();
		pointInstances.clear();
	}

	RenderQueue2D::RenderQueue2D()
	{
	}

	void RenderQueue2D::Begin(const SV::Camera* camera)
	{
		m_pLayers.clear();
		m_ViewProjectionMatrix = camera->GetViewMatrix() * camera->GetProjectionMatrix();
	}

	void RenderQueue2D::ReserveQuads(ui32 count, RenderLayer& layer)
	{
		layer.quadInstances.reserve(count);
	}
	void RenderQueue2D::DrawQuad(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, RenderLayer& layer)
	{
		XMMATRIX tm = CreateMatrix(pos, size, rot, m_ViewProjectionMatrix);
		layer.quadInstances.emplace_back(tm, col);
	}
	void RenderQueue2D::DrawQuad(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer)
	{
		XMMATRIX tm = CreateMatrix(pos, size, m_ViewProjectionMatrix);
		layer.quadInstances.emplace_back(tm, col);
	}
	void RenderQueue2D::DrawQuad(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer)
	{
		layer.quadInstances.emplace_back(XMMatrixTranspose(tm * m_ViewProjectionMatrix), col);
	}

	void RenderQueue2D::ReserveEllipse(ui32 count, RenderLayer& layer)
	{
		layer.ellipseInstances.reserve(count);
	}
	void RenderQueue2D::DrawEllipse(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, RenderLayer& layer)
	{
		XMMATRIX tm = CreateMatrix(pos, size, rot, m_ViewProjectionMatrix);
		layer.ellipseInstances.emplace_back(tm, col);
	}
	void RenderQueue2D::DrawEllipse(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer)
	{
		XMMATRIX tm = CreateMatrix(pos, size, m_ViewProjectionMatrix);
		layer.ellipseInstances.emplace_back(tm, col);
	}
	void RenderQueue2D::DrawEllipse(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer)
	{
		layer.ellipseInstances.emplace_back(XMMatrixTranspose(tm * m_ViewProjectionMatrix), col);
	}

	void RenderQueue2D::ReserveLines(ui32 count, RenderLayer& layer)
	{
		layer.lineInstances.reserve(size_t(count) * 2u);
	}
	void RenderQueue2D::DrawLine(const SV::vec2& pos0, const SV::vec2& pos1, SV::Color4b color, RenderLayer& layer)
	{
		layer.lineInstances.emplace_back(CalculateScreenSpacePos(pos0, m_ViewProjectionMatrix), color);
		layer.lineInstances.emplace_back(CalculateScreenSpacePos(pos1, m_ViewProjectionMatrix), color);
	}
	void RenderQueue2D::DrawLine(const SV::vec2& pos0, float direction, float distance, SV::Color4b color, RenderLayer& layer)
	{
		SV::vec2 pos1 = SV::vec2(cos(direction), sin(direction));
		DrawLine(pos0, pos1, color, layer);
	}

	void RenderQueue2D::ReservePoints(ui32 count, RenderLayer& layer)
	{
		layer.pointInstances.reserve(count);
	}
	void RenderQueue2D::DrawPoint(const SV::vec2& pos, SV::Color4b color, RenderLayer& layer)
	{
		layer.pointInstances.emplace_back(CalculateScreenSpacePos(pos, m_ViewProjectionMatrix), color);
	}

	void RenderQueue2D::ReserveSprites(ui32 count, RenderLayer& layer)
	{
		layer.spriteInstances.reserve(count);
	}
	void RenderQueue2D::DrawSprite(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, const Sprite& sprite, RenderLayer& layer)
	{
		XMMATRIX tm = CreateMatrix(pos, size, rot, m_ViewProjectionMatrix);
		layer.spriteInstances.emplace_back(tm, col, sprite);
	}
	void RenderQueue2D::DrawSprite(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, const Sprite& sprite, RenderLayer& layer)
	{
		XMMATRIX tm = CreateMatrix(pos, size, m_ViewProjectionMatrix);
		layer.spriteInstances.emplace_back(tm, col, sprite);
	}
	void RenderQueue2D::DrawSprite(const XMMATRIX& tm, SV::Color4b col, const Sprite& sprite, RenderLayer& layer)
	{
		XMMATRIX m = XMMatrixTranspose(tm * m_ViewProjectionMatrix);
		layer.spriteInstances.emplace_back(m, col, sprite);
	}

	void RenderQueue2D::AddLayer(RenderLayer& layer)
	{
		m_pLayers.emplace_back(&layer);
	}

	void RenderQueue2D::End()
	{
		std::sort(m_pLayers.begin(), m_pLayers.end());

		// sort sprites
		for (ui32 i = 0; i < m_pLayers.size(); ++i) {
			RenderLayer& layer = *m_pLayers[i];
			std::sort(layer.spriteInstances.begin(), layer.spriteInstances.end());
		}
	}

}