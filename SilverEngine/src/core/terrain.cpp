#include "core/terrain.h"

namespace sv {

	SV_AUX void construct_vertex_data(TerrainComponent& terrain, List<TerrainVertex>& vertices)
    {
		vertices.resize(terrain.heights.size());

		TerrainVertex* it;
		TerrainVertex* end;
		
		// HEIGHTS
		if (terrain.heights.size() == vertices.size()) {
			
			it = vertices.data();
			end = vertices.data() + vertices.size();

			const f32* heiIt = terrain.heights.data();

			while (it != end)
			{
				it->height = *heiIt;

				++heiIt;
				++it;
			}
		}
		
		// NORMALS
		if (terrain.normals.size() == vertices.size()) {
			
			it = vertices.data();
			end = vertices.data() + vertices.size();

			const v3_f32* norIt = terrain.normals.data();

			while (it != end)
			{
				it->normal = *norIt;

				++norIt;
				++it;
			}
		}
		
		// TEXCOORDS

		if (terrain.texcoords.size() == vertices.size()) {
			
			it = vertices.data();
			end = vertices.data() + vertices.size();

			const v2_f32* texIt = terrain.texcoords.data();

			while (it != end)
			{
				it->texcoord = *texIt;

				++texIt;
				++it;
			}
		}
    }

	SV_AUX bool terrain_create_buffers(TerrainComponent& terrain)
	{
		if (terrain.vbuffer || terrain.ibuffer) return false;

		List<TerrainVertex> vertex_data;
		construct_vertex_data(terrain, vertex_data);

		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Vertex;
		desc.usage = ResourceUsage_Default;
		desc.CPUAccess = CPUAccess_Write;
		desc.size = u32(vertex_data.size() * sizeof(TerrainVertex));
		desc.pData = vertex_data.data();

		SV_CHECK(graphics_buffer_create(&desc, &terrain.vbuffer));

		desc.indexType = IndexType_32;
		desc.bufferType = GPUBufferType_Index;
		desc.size = u32(terrain.indices.size() * sizeof(u32));
		desc.pData = terrain.indices.data();

		SV_CHECK(graphics_buffer_create(&desc, &terrain.ibuffer));

		graphics_name_set(terrain.vbuffer, "TerrainVertexBuffer");
		graphics_name_set(terrain.ibuffer, "TerrainIndexBuffer");

		return true;
	}

	SV_AUX void terrain_destroy_buffers(TerrainComponent& terrain)
	{
		graphics_destroy(terrain.vbuffer);
		graphics_destroy(terrain.ibuffer);
		terrain.vbuffer = NULL;
		terrain.ibuffer = NULL;
	}

	TerrainComponent::~TerrainComponent()
	{
		terrain_clear(*this);
	}
	
	void TerrainComponent::serialize(Serializer& s)
    {
		serialize_u32(s, 0u); // VERSION

		serialize_v2_u32(s, resolution);
		serialize_f32_array(s, heights);
		
		serialize_asset(s, material);
    }

    void TerrainComponent::deserialize(Deserializer& d, u32 version)
    {
		if (version == 1u) {

			terrain_clear(*this);
		
			u32 terrain_version;
			deserialize_u32(d, terrain_version); // VERSION
		
			deserialize_v2_u32(d, resolution);
			deserialize_f32_array(d, heights);
		
			deserialize_asset(d, material);
		}
    }

	bool terrain_valid(TerrainComponent& terrain)
	{
		return terrain.heights.size() == (terrain.resolution.x * terrain.resolution.y) && terrain.vbuffer != NULL && terrain.ibuffer != NULL;
	}
	
	void terrain_apply_heightmap_u8(TerrainComponent& terrain, const u8* heights, u32 size_x, u32 size_z, u32 stride)
	{
		if (size_x <= 1u || size_z <= 1u) {
			SV_LOG_ERROR("Can't apply a heightmap with dimensions smaller than 1");
			return;
		}
		if (heights == NULL) {
			SV_ASSERT(0);
			return;
		}
		
		u32 vertex_count = size_x * size_z;
		terrain.heights.reserve(vertex_count);

		// Positions

		foreach(z, size_z) {
			foreach(x, size_x) {

				f32 h = (f32(heights[(x + z * size_x) * stride]) / 255.f - 0.5f);
				terrain.heights.push_back(h);
			}
		}

		terrain.dirty = true;

		terrain.resolution.x = size_x;
		terrain.resolution.y = size_z;
	}

	bool terrain_apply_heightmap_image(TerrainComponent& terrain, const char* filepath)
	{
		u8* data;
		u32 width;
		u32 height;
		if (load_image(filepath, (void**)&data, &width, &height)) {

			terrain_apply_heightmap_u8(terrain, data, width, height, sizeof(u8) * 4u);

			SV_FREE_MEMORY(data);
			return true;
		}

		return false;
	}

	void terrain_set_flat(TerrainComponent& terrain, f32 height, u32 size_x, u32 size_z)
	{
		terrain.resolution.x = size_x;
		terrain.resolution.y = size_z;
		u32 vertex_count = size_x * size_z;

		terrain.heights.resize(vertex_count);
		foreach(i, vertex_count) terrain.heights[i] = height;

		terrain.dirty = true;
	}

	SV_AUX void terrain_calculate_texcoords(TerrainComponent& terrain)
	{
		u32 size_x = terrain.resolution.x;
		u32 size_z = terrain.resolution.y;
		
		terrain.texcoords.reset();
		terrain.texcoords.reserve(size_z * size_x);
		
		foreach(z, size_z) {
			foreach(x, size_x) {

				f32 u = f32(x) / f32(size_x);
				f32 v = f32(z) / f32(size_z);
				terrain.texcoords.emplace_back(u, v);
			}
		}
	}
	
	SV_AUX void terrain_calculate_indices(TerrainComponent& terrain)
	{
		u32 size_x = terrain.resolution.x;
		u32 size_z = terrain.resolution.y;
		
		terrain.indices.reset();
		terrain.indices.reserve((size_x - 1u) * (size_z - 1u) * 2u * 3u);
		
		foreach(z, size_z - 1u) {
			foreach(x, size_x - 1u) {

				u32 i0 = x + z * size_x;
				u32 i1 = (x + 1u) + z * size_x;
				u32 i2 = x + (z + 1u) * size_x;
				u32 i3 = (x + 1u) + (z + 1u) * size_x;

				terrain.indices.push_back(i0);
				terrain.indices.push_back(i1);
				terrain.indices.push_back(i2);

				terrain.indices.push_back(i1);
				terrain.indices.push_back(i3);
				terrain.indices.push_back(i2);
			}
		}
	}

	SV_AUX v3_f32 compute_position_from_height(f32 height, u32 index, u32 size_x, u32 size_z)
	{
		v3_f32 p;
		p.y = height;
	
		p.x = f32(index % size_x) / f32(size_x) - 0.5f;
		p.z = -(f32(index / size_x) / f32(size_z) - 0.5f);

		return p;
	}

	SV_AUX void terrain_calculate_normals(TerrainComponent& terrain)
	{
		// TODO: Optimize
		u32 size_x = terrain.resolution.x;
		u32 size_z = terrain.resolution.y;

		terrain.normals.resize(size_x * size_z);
		
		u32 i = 0u;
		u32 index_count = (size_x - 1u) * (size_z - 1u) * 2u * 3u;
		
		while (i < index_count) {

			u32 i0 = terrain.indices[i + 0];
			u32 i1 = terrain.indices[i + 1];
			u32 i2 = terrain.indices[i + 2];

			v3_f32 p0 = compute_position_from_height(terrain.heights[i0], i0, size_x, size_z);
			v3_f32 p1 = compute_position_from_height(terrain.heights[i1], i1, size_x, size_z);
			v3_f32 p2 = compute_position_from_height(terrain.heights[i2], i2, size_x, size_z);

			v3_f32 l0 = p1 - p0;
			v3_f32 l1 = p2 - p0;

			l0 = vec3_normalize(l0);
			l1 = vec3_normalize(l1);

			v3_f32 normal = vec3_cross(l0, l1);

			terrain.normals[i0] += normal;
			terrain.normals[i1] += normal;
			terrain.normals[i2] += normal;

			i += 3u;
		}

		u32 vertex_count = size_x * size_z;

		foreach(i, vertex_count) {

			terrain.normals[i] = vec3_normalize(terrain.normals[i]);
		}
	}

	void update_terrains()
	{
		ComponentIterator it;
		CompView<TerrainComponent> view;

		if (comp_it_begin(it, view)) {

			do {
				TerrainComponent& terrain = *view.comp;

				if (terrain.dirty) {

					terrain.dirty = false;
				
					terrain_calculate_texcoords(terrain);
					terrain_calculate_indices(terrain);
					terrain_calculate_normals(terrain);

					terrain_destroy_buffers(terrain);
			
					terrain_create_buffers(terrain);
				}
			}
			while (comp_it_next(it, view));
		}
	}
	
	void terrain_clear(TerrainComponent& terrain)
	{
		terrain.heights.clear();
		terrain.normals.clear();
		terrain.texcoords.clear();
		terrain.indices.clear();
		terrain.resolution.x = 0u;
		terrain.resolution.y = 0u;

		terrain_destroy_buffers(terrain);
	}

	void _terrain_register_events()
	{
		event_register("pre_draw_scene", update_terrains, 0);
	}
	
}
