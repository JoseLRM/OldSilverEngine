#pragma once

#include "core/scene.h"

namespace sv {

	struct TerrainVertex {
		f32    height;
		v3_f32 normal;
		v2_f32 texcoord;
	};

	struct TerrainComponent : public BaseComponent {

		static CompID SV_API ID;
		static constexpr u32 VERSION = 1u;

		~TerrainComponent();

		GPUBuffer* vbuffer = NULL;
		GPUBuffer* ibuffer = NULL;

		bool dirty = true;
		
		// Thats the resolution of the height samples
		v2_u32 resolution = { 50u, 50u };
		
		List<f32>    heights;
		List<v3_f32> normals;
		List<v2_f32> texcoords;

		List<u32> indices;

		v2_f32 size = { 50.f, 50.f };
		
		MaterialAsset material;

		void serialize(Serializer& s);
		void deserialize(Deserializer& s, u32 version);

    };

	SV_API void terrain_recreate(TerrainComponent& terrain, u32 size_x, u32 size_z);
	SV_API bool terrain_valid(TerrainComponent& terrain);

	SV_API void terrain_apply_heightmap_u8(TerrainComponent& terrain, const u8* heights, u32 size_x, u32 size_y, u32 stride, f32 height_mult);
	SV_API bool terrain_apply_heightmap_image(TerrainComponent& terrain, const char* filepath, f32 height_mult);

	SV_API void terrain_set_flat(TerrainComponent& terrain, f32 height);

	SV_API void terrain_clear(TerrainComponent& terrain);

	void _terrain_register_events();
	
}
