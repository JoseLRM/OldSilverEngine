#pragma once

#include "core.h"
#include "TextureAtlas.h"

namespace SV {
	typedef ui32 Tile;
}

constexpr SV::Tile SV_NULL_TILE = 0u;

namespace SV {

	class TileMap {
		struct Vertex {
			SV::vec2 position;
			SV::vec2 texCoord;
			ui32 texID;
			Vertex() = default;
			Vertex(const SV::vec2 p, const SV::vec2& tc, ui32 t)
				: texCoord(tc), position(p), texID(t) {}
		};

		struct TileData {
			Sprite sprite;
			ui32 texID;
			TileData() = default;
			TileData(const Sprite& sprite, ui32 t)
				: sprite(sprite), texID(t) {}
		};

		static Shader s_VertexShader;
		static Shader s_PixelShader;
		static InputLayout s_InputLayout;
		static Sampler s_NullSampler;
		static std::mutex s_CreateMutex;

		std::vector<TileData> m_Sprites;

		Tile* m_Tiles = nullptr;
		ui32 m_Width = 0u;
		ui32 m_Height = 0u;

		static constexpr ui32 MAX_TEXTURES = 10;
		TextureAtlas* m_Textures[MAX_TEXTURES];

		VertexBuffer m_VertexBuffer;
		IndexBuffer m_IndexBuffer;
		ConstantBuffer m_ConstantBuffer;
		ui32 m_IndexCount = 0u;
		bool m_Modified = true;

	public:
		TileMap();
		~TileMap();
		TileMap(const TileMap& tileMap) = delete;
		TileMap(TileMap&& tileMap) noexcept;
		
		TileMap& operator=(const TileMap& other) = delete;
		TileMap& operator=(TileMap&& other) noexcept;

		void CreateGrid(ui32 width, ui32 height);
		Tile CreateTile(const SV::Sprite& sprite);
		void PutTile(i32 x, i32 y, Tile tile) noexcept;

		// Getters
		inline ui32 GetWidth() const noexcept { return m_Width; }
		inline ui32 GetHeight() const noexcept { return m_Height; }
		inline bool InBounds(i32 x, i32 y) const noexcept { return x < m_Width && x >= 0 && y < m_Height && y >= 0; }
		inline ui32 GetTilesCount() const noexcept { return ui32(m_Sprites.size()); }
		inline Sprite GetSprite(ui32 ID) const noexcept { return m_Sprites[ID].sprite; }

		// Graphics Methods
		void Bind(XMMATRIX matrix, CommandList& cmd);
		static void BindShaders(CommandList& cmd);
		void Draw(CommandList& cmd);
		void Unbind(CommandList& cmd);
		static void UnbindShaders(CommandList& cmd);

		static bool CreateShaders(Graphics& graphics);

	private:
		void Allocate(ui32 width, ui32 height);
		void Deallocate();

		bool CreateBuffers(Graphics& graphics);

	};

}