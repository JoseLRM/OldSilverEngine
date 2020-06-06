#include "core.h"

#include "RenderQueue2D.h"

namespace SV {

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
		XMMATRIX tm = XMMatrixTranspose(XMMatrixScaling(size.x, size.y, 0.f) * XMMatrixRotationZ(rot) 
			* XMMatrixTranslation(pos.x, pos.y, 0.f) * m_ViewProjectionMatrix);
		layer.quadInstances.emplace_back(tm, col);
	}
	void RenderQueue2D::DrawQuad(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer)
	{
		XMMATRIX tm = XMMatrixTranspose(XMMatrixScaling(size.x, size.y, 0.f) * XMMatrixTranslation(pos.x, pos.y, 0.f) * m_ViewProjectionMatrix);
		layer.quadInstances.emplace_back(tm, col);
	}
	void RenderQueue2D::DrawQuad(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer)
	{
		layer.quadInstances.emplace_back(XMMatrixTranspose(tm * m_ViewProjectionMatrix), col);
	}

	void RenderQueue2D::ReserveEllipse(ui32 count, RenderLayer& layer)
	{
	}
	void RenderQueue2D::DrawEllipse(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, RenderLayer& layer)
	{
	}
	void RenderQueue2D::DrawEllipse(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer)
	{
	}
	void RenderQueue2D::DrawEllipse(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer)
	{
	}

	void RenderQueue2D::ReserveLines(ui32 count, RenderLayer& layer)
	{
	}
	void RenderQueue2D::DrawLine(const SV::vec2& pos0, const SV::vec2& pos1, SV::Color4b, RenderLayer& layer)
	{
	}
	void RenderQueue2D::DrawLine(const SV::vec2& pos, float direction, float distance, SV::Color4b, RenderLayer& layer)
	{
	}

	void RenderQueue2D::ReservePoints(ui32 count, RenderLayer& layer)
	{
	}
	void RenderQueue2D::DrawPoint(const SV::vec2& pos, RenderLayer& layer)
	{
	}

	void RenderQueue2D::AddLayer(RenderLayer& layer)
	{
		m_pLayers.emplace_back(&layer);
	}

	void RenderQueue2D::End()
	{
		std::sort(m_pLayers.begin(), m_pLayers.end());
	}

}