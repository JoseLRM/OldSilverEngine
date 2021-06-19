#include "core/terrain.h"

#include "debug/editor.h"

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

	SV_AUX void terrain_update_buffers(TerrainComponent& terrain, CommandList cmd)
	{
		if (terrain.vbuffer == NULL || terrain.ibuffer == NULL) return;

		List<TerrainVertex> vertex_data;
		construct_vertex_data(terrain, vertex_data);

		graphics_buffer_update(terrain.vbuffer, vertex_data.data(), (u32)vertex_data.size() * sizeof(TerrainVertex), 0u, cmd);
		graphics_buffer_update(terrain.ibuffer, terrain.indices.data(), (u32)terrain.indices.size() * sizeof(u32), 0u, cmd);
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

			dirty = true;
		}
    }

	void terrain_recreate(TerrainComponent& terrain, u32 size_x, u32 size_z)
	{
		if (size_x < 2 || size_z < 2) {
			SV_LOG_ERROR("Can't resize the terrain with dimensions less than 2");
			return;
		}
		
		terrain.resolution.x = size_x;
		terrain.resolution.y = size_z;
		terrain.dirty = true;

		terrain_set_flat(terrain, 0.f);
		terrain_destroy_buffers(terrain);

		terrain.size = { f32(size_x), f32(size_z) };
	}

	bool terrain_valid(TerrainComponent& terrain)
	{
		return !terrain.dirty && terrain.heights.size() == (terrain.resolution.x * terrain.resolution.y) && terrain.vbuffer != NULL && terrain.ibuffer != NULL;
	}

	SV_AUX u8 sample_image_linear(v2_f32 coord, const u8* buff, u32 width, u32 height, u32 stride)
	{
		i32 x0 = (i32)floor(coord.x * f32(width));
		i32 x1 = x0 + 1;
		i32 y0 = (i32)floor(coord.y * f32(height));
		i32 y1 = y0 + 1;

		f32 px = (coord.x * f32(width)) - f32(x0);
		f32 py = (coord.y * f32(height)) - f32(y0);

		// TODO: This can exceed the buffer
		f32 s0 = f32(buff[(x0 + y0 * width) * stride]);
		f32 s1 = f32(buff[(x1 + y0 * width) * stride]);
		f32 s2 = f32(buff[(x0 + y1 * width) * stride]);
		f32 s3 = f32(buff[(x1 + y1 * width) * stride]);

		f32 v0 = s0 * (1.f - px) + s1 * px;
		f32 v1 = s2 * (1.f - px) + s3 * px;

		return u8(v0 * (1.f - py) + v1 * py);
	}

	SV_AUX u8 sample_image_nearest(v2_f32 coord, const u8* buff, u32 width, u32 height, u32 stride)
	{
		i32 x = (i32)floor(coord.x * f32(width));
		i32 y = (i32)floor(coord.y * f32(height));

		return buff[(x + y * width) * stride];
	}
	
	void terrain_apply_heightmap_u8(TerrainComponent& terrain, const u8* heights, u32 size_x, u32 size_z, u32 stride, f32 height_mult)
	{
		if (size_x <= 1u || size_z <= 1u) {
			SV_LOG_ERROR("Can't apply a heightmap with dimensions smaller than 1");
			return;
		}
		if (heights == NULL) {
			SV_ASSERT(0);
			return;
		}

		terrain.heights.reset();
		terrain.heights.reserve(terrain.resolution.x * terrain.resolution.y);

		// Positions

		foreach(z, terrain.resolution.y) {
			foreach(x, terrain.resolution.x) {

				f32 u = f32(x) / (f32)terrain.resolution.x;
				f32 v = f32(z) / (f32)terrain.resolution.y;

				u8 sample = sample_image_linear({ u, v }, heights, size_x, size_z, stride);
				f32 h = f32(sample) / 255.f * height_mult;
				terrain.heights.push_back(h);
			}
		}

		terrain.dirty = true;
	}

	bool terrain_apply_heightmap_image(TerrainComponent& terrain, const char* filepath, f32 height_mult)
	{
		u8* data;
		u32 width;
		u32 height;
		if (load_image(filepath, (void**)&data, &width, &height)) {

			terrain_apply_heightmap_u8(terrain, data, width, height, sizeof(u8) * 4u, height_mult);

			SV_FREE_MEMORY(data);
			return true;
		}

		return false;
	}

	void terrain_set_flat(TerrainComponent& terrain, f32 height)
	{
		u32 vertex_count = terrain.resolution.x * terrain.resolution.y;
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

		terrain.normals.reset();
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
		CompID terrain_id = get_component_id("Terrain");

		for (CompIt it = comp_it_begin(terrain_id);
			 it.has_next;
			 comp_it_next(it))
		{
			TerrainComponent& terrain = *(TerrainComponent*)it.comp;

			if (terrain.dirty) {
				
				terrain.dirty = false;

				if (terrain.vbuffer == NULL || terrain.ibuffer == NULL) {

					if (terrain.heights.size() != terrain.resolution.x * terrain.resolution.y) {
						terrain_set_flat(terrain, 0.f);
					}
						
					terrain_calculate_texcoords(terrain);
					terrain_calculate_indices(terrain);
					terrain_calculate_normals(terrain);

					terrain_create_buffers(terrain);
				}
				else {

					terrain_calculate_normals(terrain);

					CommandList cmd = graphics_commandlist_get();
						
					terrain_update_buffers(terrain, cmd);
				}
			}
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

#if SV_EDITOR

	SV_INTERNAL void display_terrain_component_data(DisplayComponentEvent* e)
	{
		if (get_component_id("Terrain") == e->comp_id) {
				
			TerrainComponent& t = *reinterpret_cast<TerrainComponent*>(e->comp);

			if (t.material.get() == NULL) {
				create_asset(t.material, "Material");
			}

			// TEMP
			{
				if (gui_button("Move")) {
					
					foreach(z, t.resolution.y) {
						foreach(x, t.resolution.x) {
							t.heights[x + z * t.resolution.x] = sin(f32(x) * 0.06f) * (f32(z) / f32(t.resolution.y)) * 0.1f;
						}
					}

					t.dirty = true;
				}
				if (gui_button("Apply heightmap")) {

					gui_show_window("Apply Heightmap");
				}
			}

			egui_comp_texture("Diffuse", 0u, &t.material->diffuse_map);

			if (gui_button("Recreate")) {

				gui_show_window("Recreate Terrain");
			}
		}
	}

	SV_AUX TerrainComponent* get_selected_terrain()
	{
		List<Entity> entities = editor_selected_entities();
		
		if (entities.size() != 1u)
			return NULL;
		
		Entity entity = entities.back();
				
		return (TerrainComponent*)get_entity_component(entity, get_component_id("Terrain"));
	}

	SV_INTERNAL void display_terrain_gui()
	{
		if (gui_begin_window("Recreate Terrain", GuiWindowFlag_Temporal)) {

			static v2_u32 resolution = { 50u, 50u };

			bool close = false;

			TerrainComponent* terrain = get_selected_terrain();

			if (terrain == NULL)
				close = true;
			else {
				
				gui_drag_u32("ResolutionX", resolution.x, 1u, 2u, 10000);
				gui_drag_u32("ResolutionY", resolution.y, 1u, 2u, 10000);

				if (gui_button("Recreate")) {

					terrain_recreate(*terrain, resolution.x, resolution.y);
					close = true;
				}
			}

			if (close) {

				resolution = { 50u, 50u };
				gui_hide_window("Recreate Terrain");
			}

			gui_end_window();
		}
		
		if (gui_begin_window("Apply Heightmap", GuiWindowFlag_Temporal)) {

			// TODO: Filepath

			static f32 height_mult = 10.f;

			bool close = false;
			TerrainComponent* t = get_selected_terrain();

			if (t == NULL) close = true;
			else {

				gui_drag_f32("Height", height_mult, 0.1f, 0.f, f32_max);

				if (gui_button("Apply")) {

					terrain_apply_heightmap_image(*t, "assets/images/pene.JPG", height_mult);
					close = true;
				}
			}

			if (close) {

				height_mult = 10.f;
				gui_hide_window("Apply Heightmap");
			}

			gui_end_window();
		}
	}
	
#endif

	void _terrain_register_events()
	{
		event_register("pre_draw_scene", update_terrains, 0);

#if SV_EDITOR
		event_register("display_component_data", display_terrain_component_data, 0);
		event_register("display_gui", display_terrain_gui, 0);
#endif
	}
	
}
