#pragma once

#include "core.h"
#include "TextureAtlas.h"

namespace SV {

	struct SpriteInstance {
		XMMATRIX tm;
		SV::Color4b color;
		Sprite sprite;
		SpriteInstance(const XMMATRIX& tm, SV::Color4b col, Sprite spr)
			: tm(tm), color(col), sprite(spr) {}
		inline bool operator<(const SpriteInstance& other) const noexcept
		{
			return sprite.pTextureAtlas < other.sprite.pTextureAtlas;
		}
	};
	struct QuadInstance {
		XMMATRIX tm;
		SV::Color4b color;
		QuadInstance(const XMMATRIX& tm, SV::Color4b col)
			: tm(tm), color(col) {}
	};
	struct PointInstance {
		SV::vec2 pos;
		SV::Color4b color;
		PointInstance(const SV::vec2& pos, SV::Color4b color) 
			: pos(pos), color(color) {}
	};

	class RenderQueue2D;
	class Renderer;
	class Renderer2D;
	class Camera;

	class RenderLayer {
		i32 m_Value = 0;
		bool m_Transparent = false;
		std::vector<SpriteInstance> spriteInstances;
		std::vector<QuadInstance> quadInstances;
		std::vector<QuadInstance> ellipseInstances;
		std::vector<PointInstance> lineInstances;
		std::vector<PointInstance> pointInstances;

	public:
		friend RenderQueue2D;
		friend Renderer;
		friend Renderer2D;

		RenderLayer() = default;
		RenderLayer(i32 value, bool transparent) 
			: m_Value(value), m_Transparent(transparent) {}

		inline void SetValue(i32 value) noexcept { m_Value = value; }
		inline void SetTransparent(bool trans) noexcept { m_Transparent = trans; }

		inline i32 GetValue() const noexcept { return m_Value; }
		inline bool IsTransparent() const noexcept { return m_Transparent; }
		void Reset() noexcept;

		bool operator<(const RenderLayer& other) const noexcept
		{
			if (m_Transparent) {
				if (m_Value != other.m_Value) return m_Value < other.m_Value;
				else if (m_Transparent != other.m_Transparent) return !m_Transparent;
				else return false;
			}
			else {
				if (m_Value != other.m_Value) return m_Value > other.m_Value;
				else if (m_Transparent != other.m_Transparent) return !m_Transparent;
				else return false;
			}
		}

	};

	class RenderQueue2D {
		std::vector<RenderLayer*> m_pLayers;

		XMMATRIX m_ViewProjectionMatrix;

	public:
		friend Renderer;
		friend Renderer2D;
		RenderQueue2D();

		void Begin(const SV::Camera* camera);

		void ReserveQuads(ui32 count, RenderLayer& layer);
		void DrawQuad(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, RenderLayer& layer);
		void DrawQuad(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer);
		void DrawQuad(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer);
		
		void ReserveEllipses(ui32 count, RenderLayer& layer);
		void DrawEllipse(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, RenderLayer& layer);
		void DrawEllipse(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, RenderLayer& layer);
		void DrawEllipse(const XMMATRIX& tm, SV::Color4b col, RenderLayer& layer);

		void ReserveLines(ui32 count, RenderLayer& layer);
		void DrawLine(const SV::vec2& pos0, const SV::vec2& pos1, SV::Color4b, RenderLayer& layer);
		void DrawLine(const SV::vec2& pos, float direction, float distance, SV::Color4b, RenderLayer& layer);

		void ReservePoints(ui32 count, RenderLayer& layer);
		void DrawPoint(const SV::vec2& pos, SV::Color4b color, RenderLayer& layer);

		void ReserveSprites(ui32 count, RenderLayer& layer);
		void DrawSprite(const SV::vec2& pos, const SV::vec2& size, float rot, SV::Color4b col, const Sprite& sprite, RenderLayer& layer);
		void DrawSprite(const SV::vec2& pos, const SV::vec2& size, SV::Color4b col, const Sprite& sprite, RenderLayer& layer);
		void DrawSprite(const XMMATRIX& tm, SV::Color4b col, const Sprite& sprite, RenderLayer& layer);

		void AddLayer(RenderLayer& layer);

		void End();

	};

}