#pragma once

#include "core.h"

namespace SV {

	struct SpriteInstance {
		SV::Color4b color;
		SV::vec4 texCoord;
		XMMATRIX tm;
		void* texture;
	};
	struct QuadInstance {
		XMMATRIX tm;
		SV::Color4b color;
		QuadInstance(const XMMATRIX& tm, SV::Color4b col)
			: tm(tm), color(col) {}
	};
	struct LineInstance {
		SV::Color4b color;
		SV::vec2 pos0;
		SV::vec2 pos1;
	};
	struct PointInstance {
		SV::vec2 pos;
		SV::Color4b color;
	};

	class RenderQueue2D;
	class Renderer;
	class Camera;

	class RenderLayer {
		i32 m_Value = 0;
		bool m_Transparent = false;
		std::vector<SpriteInstance> spriteInstances;
		std::vector<QuadInstance> quadInstances;
		std::vector<QuadInstance> ellipseInstances;
		std::vector<LineInstance> lineInstances;
		std::vector<PointInstance> pointInstances;

	public:
		friend RenderQueue2D;
		friend Renderer;

		RenderLayer() = default;
		RenderLayer(i32 value, bool transparent) 
			: m_Value(value), m_Transparent(transparent) {}

		inline i32 GetValue() const noexcept { return m_Value; }
		inline bool IsTransparent() const noexcept { return m_Transparent; }
		void Reset() noexcept;

		bool operator<(const RenderLayer& other) const noexcept
		{
			if (m_Transparent != other.m_Transparent) return m_Value < other.m_Value;
			else return !m_Transparent;
		}

	};

	class RenderQueue2D {
		std::vector<RenderLayer*> m_pLayers;

		XMMATRIX m_ViewProjectionMatrix;

	public:
		friend Renderer;
		RenderQueue2D();

		void Begin(const SV::Camera* camera);

		void ReserveQuads(ui32 count, RenderLayer& layer);
		void DrawQuad(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, RenderLayer& layer);
		void DrawQuad(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer);
		void DrawQuad(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer);
		
		void ReserveEllipse(ui32 count, RenderLayer& layer);
		void DrawEllipse(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, RenderLayer& layer);
		void DrawEllipse(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer);
		void DrawEllipse(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer);

		void ReserveLines(ui32 count, RenderLayer& layer);
		void DrawLine(const SV::vec2& pos0, const SV::vec2& pos1, SV::Color4b, RenderLayer& layer);
		void DrawLine(const SV::vec2& pos, float direction, float distance, SV::Color4b, RenderLayer& layer);

		void ReservePoints(ui32 count, RenderLayer& layer);
		void DrawPoint(const SV::vec2& pos, RenderLayer& layer);

		//void ReserveSprites(ui32 count, SpriteLayer& layer);
		//void DrawSprite(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, SpriteLayer& layer);
		//void DrawSprite(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, SpriteLayer& layer);
		//void DrawSprite(const XMMATRIX& tm, SV::Color4b col, SpriteLayer& layer);

		void AddLayer(RenderLayer& layer);

		void End();

	};

}