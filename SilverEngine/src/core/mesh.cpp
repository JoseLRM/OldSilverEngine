#include "core/mesh.h"

#include <sstream>

#define ASSERT_VERTICES() SV_ASSERT(mesh.positions.size() == mesh.normals.size() && mesh.positions.size() == mesh.tangents.size() && (mesh.texcoords.empty() || mesh.positions.size() == mesh.texcoords.size()))

namespace sv {

    /*
      Vertices: 4
      Indices: 6
    */
    SV_AUX void computePlane(Mesh& mesh, const XMMATRIX& transformMatrix, const XMVECTOR& rotationQuaternion, size_t vertexOffset, size_t indexOffset)
    {
		XMVECTOR v0, v1, v2, v3;

		// Positions
		v0 = XMVector3Transform(XMVectorSet(-0.5f, 0.f, 0.5f, 0.f), transformMatrix);
		v1 = XMVector3Transform(XMVectorSet(0.5f, 0.f, 0.5f, 0.f), transformMatrix);
		v2 = XMVector3Transform(XMVectorSet(-0.5f, 0.f, -0.5f, 0.f), transformMatrix);
		v3 = XMVector3Transform(XMVectorSet(0.5f, 0.f, -0.5f, 0.f), transformMatrix);

		v3_f32* pos = mesh.positions.data() + vertexOffset;
		*pos = v0; ++pos;
		*pos = v1; ++pos;
		*pos = v2; ++pos;
		*pos = v3;

		// Normals
		v0 = XMVector3Rotate(XMVectorSet(0.f, 1.f, 0.f, 0.f), rotationQuaternion);

		v3_f32* nor = mesh.normals.data() + vertexOffset;
		*nor = v0; ++nor;
		*nor = v0; ++nor;
		*nor = v0; ++nor;
		*nor = v0;

		// Indices
		u32* ind = mesh.indices.data();
		u32 v0_32 = u32(vertexOffset);

		ind[indexOffset + 0u] = v0_32 + 0u;
		ind[indexOffset + 1u] = v0_32 + 1u;
		ind[indexOffset + 2u] = v0_32 + 2u;
		ind[indexOffset + 3u] = v0_32 + 1u;
		ind[indexOffset + 4u] = v0_32 + 3u;
		ind[indexOffset + 5u] = v0_32 + 2u;
    }

    SV_AUX void construct_vertex_data(Mesh& mesh, List<MeshVertex>& vertices)
    {
		vertices.resize(mesh.positions.size());

		MeshVertex* it;
		MeshVertex* end;
		// POSITIONS
		if (mesh.positions.size() == vertices.size()) {
			it = vertices.data();
			end = vertices.data() + vertices.size();

			const v3_f32* posIt = mesh.positions.data();

			while (it != end)
			{
				it->position = *posIt;

				++posIt;
				++it;
			}
		}
		// NORMALS
		if (mesh.normals.size() == vertices.size()) {
	    
			it = vertices.data();
			end = vertices.data() + vertices.size();

			const v3_f32* norIt = mesh.normals.data();

			while (it != end)
			{
				it->normal = *norIt;

				++norIt;
				++it;
			}
		}
		// TANGENTS
		if (mesh.tangents.size() == vertices.size()) {
	    
			it = vertices.data();
			end = vertices.data() + vertices.size();

			const v4_f32* tanIt = mesh.tangents.data();

			while (it != end)
			{
				it->tangents = *tanIt;

				++tanIt;
				++it;
			}
		}
		// TEX COORDS
		if (mesh.texcoords.size() == vertices.size()) {
	    
			it = vertices.data();
			end = vertices.data() + vertices.size();

			const v2_f32* texIt = mesh.texcoords.data();

			while (it != end)
			{
				it->texcoord = *texIt;

				++texIt;
				++it;
			}
		}
    }

    void mesh_apply_plane(Mesh& mesh, const XMMATRIX& transform)
    {
		ASSERT_VERTICES();
		
		size_t vertexOffset = mesh.positions.size();
		size_t indexOffset = mesh.indices.size();

		mesh.positions.resize(vertexOffset + 4u);
		mesh.normals.resize(vertexOffset + 4u);

		mesh.indices.resize(indexOffset + 6u);

		computePlane(mesh, transform, XMQuaternionRotationMatrix(transform), vertexOffset, indexOffset);
    }

    void mesh_apply_cube(Mesh& mesh, const XMMATRIX& transform)
    {
		ASSERT_VERTICES();

		size_t vertexOffset = mesh.positions.size();
		size_t indexOffset = mesh.indices.size();

		mesh.positions.resize(vertexOffset + 6u * 4u);
		mesh.normals.resize(vertexOffset + 6u * 4u);

		mesh.indices.resize(indexOffset + 6u * 6u);

		XMMATRIX faceTransform, faceRotation;
		XMVECTOR quat;

		// UP
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * transform;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(mesh, faceTransform, quat, vertexOffset + 4u * 0u, indexOffset + 6 * 0u);

		// DOWN
		faceRotation = XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(180.f));
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transform;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(mesh, faceTransform, quat, vertexOffset + 4u * 1u, indexOffset + 6 * 1u);

		// LEFT
		faceRotation = XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(-90.f));
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transform;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(mesh, faceTransform, quat, vertexOffset + 4u * 2u, indexOffset + 6 * 2u);

		// RIGHT
		faceRotation = XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(90.f));
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transform;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(mesh, faceTransform, quat, vertexOffset + 4u * 3u, indexOffset + 6 * 3u);

		// FRONT
		faceRotation = XMMatrixRotationRollPitchYaw(ToRadians(-90.f), 0.f, 0.f);
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transform;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(mesh, faceTransform, quat, vertexOffset + 4u * 4u, indexOffset + 6 * 4u);

		// BACK
		faceRotation = XMMatrixRotationRollPitchYaw(ToRadians(90.f), 0.f, 0.f);
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transform;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(mesh, faceTransform, quat, vertexOffset + 4u * 5u, indexOffset + 6 * 5u);
    }

    void mesh_apply_sphere(Mesh& mesh, u32 resolution, const XMMATRIX& transform)
    {
		u32 begin_vertex = (u32)mesh.positions.size();
		u32 begin_index = (u32)mesh.indices.size();
		
		f32 res_mult = 1.f / (resolution - 1u);

		u32 vertex_count = resolution * resolution;
		u32 index_count = (resolution - 1u) * (resolution - 1u) * 2u * 3u;

		mesh.positions.resize(begin_vertex + vertex_count * 6u);
		mesh.normals.resize(begin_vertex + vertex_count * 6u);

		mesh.indices.resize(begin_index + index_count * 6u);

		XMMATRIX matrix = XMMatrixIdentity();

		v3_f32 center = XMVector4Transform(XMVectorSet(0.f, 0.f, 0.f, 1.f), transform);

		foreach(face, 6u) {

			u32 vertex_offset = begin_vertex + (vertex_count * face);
			u32 index_offset = begin_index + (index_count * face);

			switch (face) {

			case 0: // UP
				matrix = XMMatrixTranslation(0.f, 0.5f, 0.f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * transform;
				break;

			case 1: // DOWN
				matrix = XMMatrixTranslation(0.f, 0.5f, 0.f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(180.f)) * transform;
				break;

			case 2: // LEFT
				matrix = XMMatrixTranslation(0.f, 0.5f, 0.f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(-90.f)) * transform;
				break;

			case 3: // RIGHT
				matrix = XMMatrixTranslation(0.f, 0.5f, 0.f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(90.f)) * transform;
				break;

			case 4: // FRONT
				matrix = XMMatrixTranslation(0.f, 0.5f, 0.f) * XMMatrixRotationRollPitchYaw(ToRadians(-90.f), 0.f, 0.f) * transform;
				break;

			case 5: // BACK
				matrix = XMMatrixTranslation(0.f, 0.5f, 0.f) * XMMatrixRotationRollPitchYaw(ToRadians(90.f), 0.f, 0.f) * transform;
				break;
				
			}
			
			foreach(y, resolution) {
				foreach(x, resolution) {

					u32 index = vertex_offset + x + (y * resolution);
					v3_f32& pos = mesh.positions[index];
					v3_f32& nor = mesh.normals[index];
					
					pos.x = f32(x) * res_mult - 0.5f;
					pos.y = 0.f;
					pos.z = f32(y) * res_mult - 0.5f;
					
					pos = XMVector4Transform(vec3_to_dx(pos, 1.f), matrix);
					nor = vec3_normalize(pos - center);
					pos = center + nor * 0.5f;
				}
			}

			foreach(y, resolution - 1u) {
				foreach(x, resolution - 1u) {

					u32 i0 = vertex_offset + (x + 0) + (y + 1) * resolution;
					u32 i1 = vertex_offset + (x + 1) + (y + 1) * resolution;
					u32 i2 = vertex_offset + (x + 0) + (y + 0) * resolution;
					u32 i3 = vertex_offset + (x + 1) + (y + 0) * resolution;

					mesh.indices[index_offset++] = i0;
					mesh.indices[index_offset++] = i1;
					mesh.indices[index_offset++] = i2;
					mesh.indices[index_offset++] = i1;
					mesh.indices[index_offset++] = i3;
					mesh.indices[index_offset++] = i2;
				}
			}
		}
    }

    void mesh_set_scale(Mesh& mesh, f32 scale, bool center)
    {
		v3_f32 min, max;
		min.x = f32_max;
		min.y = f32_max;
		max.x = f32_min;
		max.y = f32_min;

		// Compute bounds
		for (const v3_f32& pos : mesh.positions) {

			if (pos.x < min.x) min.x = pos.x;
			if (pos.x > max.x) max.x = pos.x;
			if (pos.y < min.y) min.y = pos.y;
			if (pos.y > max.y) max.y = pos.y;
			if (pos.z < min.z) min.z = pos.z;
			if (pos.z > max.z) max.z = pos.z;
		}

		// Scale mesh
		v3_f32 dimensions = max - min;
		f32 inv_dim = 1.f / SV_MAX(SV_MAX(dimensions.x, dimensions.y), dimensions.z);

		for (v3_f32& pos : mesh.positions) {

			pos = (((pos - min) * inv_dim) - 0.5f) * scale;
		}

		// Center
		if (center) {

			max = (((max - min) * inv_dim) - 0.5f) * scale;
			min = -0.5f * scale;

			v3_f32 addition = (min + (max - min) * 0.5f);

			for (v3_f32& pos : mesh.positions) {
				pos -= addition;
			}
		}
    }

    void mesh_optimize(Mesh& mesh)
    {
		SV_LOG_ERROR("TODO");
    }

	SV_AUX void calculate_normals(u32* indices, v3_f32* positions, v3_f32* normals, u32 vertex_count, u32 index_count)
	{
		// TODO: Optimize
		
		u32 i = 0u;
		
		while (i < index_count) {

			u32 i0 = indices[i + 0];
			u32 i1 = indices[i + 1];
			u32 i2 = indices[i + 2];

			v3_f32 p0 = positions[i0];
			v3_f32 p1 = positions[i1];
			v3_f32 p2 = positions[i2];

			v3_f32 l0 = p1 - p0;
			v3_f32 l1 = p2 - p0;

			l0 = vec3_normalize(l0);
			l1 = vec3_normalize(l1);

			v3_f32 normal = vec3_cross(l0, l1);

			normals[i0] += normal;
			normals[i1] += normal;
			normals[i2] += normal;

			i += 3u;
		}

		foreach(i, vertex_count) {

			normals[i] = vec3_normalize(normals[i]);
		}
	}

	void mesh_calculate_normals(Mesh& mesh)
	{
		mesh.normals.resize(mesh.positions.size());
		calculate_normals(mesh.indices.data(), mesh.positions.data(), mesh.normals.data(), (u32)mesh.positions.size(), (u32)mesh.indices.size());
	}
	
	void mesh_calculate_tangents(Mesh& mesh)
	{
		if (mesh.texcoords.size() != mesh.positions.size())
			return;
		
		mesh.tangents.resize(mesh.positions.size());
	    
		MeshIndex* it = mesh.indices.data();
		MeshIndex* end = mesh.indices.data() + mesh.indices.size();

		while (it < end) {

			MeshIndex i0 = *(it + 0);
			MeshIndex i1 = *(it + 1);
			MeshIndex i2 = *(it + 2);
		
			v3_f32 pos0 = mesh.positions[i0];
			v3_f32 pos1 = mesh.positions[i1];
			v3_f32 pos2 = mesh.positions[i2];

			v2_f32 tc0 = mesh.texcoords[i0];
			v2_f32 tc1 = mesh.texcoords[i1];
			v2_f32 tc2 = mesh.texcoords[i2];

			v3_f32 normal = (mesh.normals[i0] + mesh.normals[i1] + mesh.normals[i2]) / 3.f;

			v3_f32 edge0 = pos1 - pos0;
			v3_f32 edge1 = pos2 - pos0;
		
			v2_f32 deltaUV0 = tc1 - tc0;
			v2_f32 deltaUV1 = tc2 - tc0;

			f32 f = 1.f / (deltaUV0.x * deltaUV1.y - deltaUV1.x * deltaUV0.y);

			v4_f32 tan;
			tan.x = f * (deltaUV1.y * edge0.x + deltaUV0.y * edge1.x);
			tan.y = f * (deltaUV1.y * edge0.y + deltaUV0.y * edge1.y);
			tan.z = f * (deltaUV1.y * edge0.z + deltaUV0.y * edge1.z);
			tan.w = 1.f;

			// TODO: I'm losing tangent precission here
			mesh.tangents[i0] = tan;
			mesh.tangents[i1] = tan;
			mesh.tangents[i2] = tan;
		
			it += 3u;
		}
	}

    void mesh_recalculate_normals_and_tangents(Mesh& mesh)
    {
		SV_LOG_ERROR("TODO");
    }

    bool mesh_create_buffers(Mesh& mesh, ResourceUsage usage)
    {
		//ASSERT_VERTICES();
		SV_ASSERT(usage != ResourceUsage_Staging);
		if (mesh.vbuffer || mesh.ibuffer) return false;

		List<MeshVertex> vertex_data;
		construct_vertex_data(mesh, vertex_data);

		GPUBufferDesc desc;
		desc.buffer_type = GPUBufferType_Vertex;
		desc.usage = usage;
		desc.cpu_access = (usage == ResourceUsage_Static) ? CPUAccess_None : CPUAccess_Write;
		desc.size = u32(vertex_data.size() * sizeof(MeshVertex));
		desc.data = vertex_data.data();

		SV_CHECK(graphics_buffer_create(&desc, &mesh.vbuffer));

		desc.index_type = IndexType_32;
		desc.buffer_type = GPUBufferType_Index;
		desc.size = u32(mesh.indices.size() * sizeof(MeshIndex));
		desc.data = mesh.indices.data();

		SV_CHECK(graphics_buffer_create(&desc, &mesh.ibuffer));

		graphics_name_set(mesh.vbuffer, "MeshVertexBuffer");
		graphics_name_set(mesh.ibuffer, "MeshIndexBuffer");

		return true;
    }

    bool mesh_update_buffers(Mesh& mesh, CommandList cmd)
    {
		if (mesh.vbuffer == NULL || mesh.ibuffer == NULL) {

			SV_CHECK(mesh_create_buffers(mesh, ResourceUsage_Dynamic));
		}

		List<MeshVertex> vertex_data;
		construct_vertex_data(mesh, vertex_data);

		graphics_buffer_update(mesh.vbuffer, GPUBufferState_Vertex, vertex_data.data(), (u32)vertex_data.size() * sizeof(MeshVertex), 0u, cmd);
		graphics_buffer_update(mesh.ibuffer, GPUBufferState_Index, mesh.indices.data(), (u32)mesh.indices.size() * sizeof(MeshIndex), 0u, cmd);
		
		return true;
    }

	void mesh_destroy_buffers(Mesh& mesh)
    {
		graphics_destroy(mesh.vbuffer);
		graphics_destroy(mesh.ibuffer);
		mesh.vbuffer = NULL;
		mesh.ibuffer = NULL;
    }

    void mesh_clear(Mesh& mesh)
    {
		mesh.positions.clear();
		mesh.normals.clear();

		mesh.indices.clear();

		mesh_destroy_buffers(mesh);
    }

    SV_AUX void set_default_material(MaterialInfo* mat)
    {
		// Defaults
		mat->ambient_color = color_float(0.2f, 0.2f, 0.2f, 1.f);
		mat->diffuse_color = color_float(0.8f, 0.8f, 0.8f, 1.f);
		mat->specular_color = color_float(1.f, 1.f, 1.f, 1.f);
		mat->emissive_color = color_float(0.f, 0.f, 0.f, 1.f);
		mat->shininess = 1.f;
    }

    SV_AUX void read_mtl(const char* filepath, ModelInfo& model_info)
    {
		String file;

		if (file_read_text(filepath, file)) {

			MaterialInfo* mat = &model_info.materials.emplace_back();
			set_default_material(mat);
			bool use_default = true;
	    
			LineProcessor p;
			line_begin(p, file.c_str());

			bool corrupted = false;

			while (line_next(p) && !corrupted) {

				line_jump_spaces(p.line);

				switch (*p.line) {

				case 'm':
				{
					++p.line;

					if (p.line[0] == 'a' && p.line[1] == 'p' && p.line[2] == '_') {

						p.line += 3u;

						String* str = nullptr;

						if (p.line[0] == 'K' && p.line[1] == 'd') { // Diffuse map
							str = &mat->diffuse_map_path;
						}
						else if (p.line[0] == 'K' && p.line[1] == 's') { // Specular map
							str = &mat->specular_map_path;
						}
						else if (p.line[0] == 'K' && p.line[1] == 'a') { // TODO: Ambient map
							str = nullptr;
						}
						else if (p.line[0] == 'K' && p.line[1] == 'n') { // Normal map
							str = &mat->normal_map_path;
						}

						if (str) {

							p.line += 2;

							line_jump_spaces(p.line);

							char delimiters[] = {
								' ',
								'\n',
								'\r'
							};
		    
							size_t name_size = string_split(p.line, delimiters, 3u);
							if (name_size == 0u) {
			
								SV_LOG_ERROR("Can't read the mtl texture path in line %u", p.line_count);
								corrupted = true;
							}
							else {

								str->set(p.line, 0u, name_size);
								path_clear(str->c_str());
							}
						}
					}
				}
				break;

				case 'i':
				{
					++p.line;

					if (p.line[0] == 'l' && p.line[1] == 'l' && p.line[2] == 'u' && p.line[3] == 'm') {

						p.line += 4u;

						line_jump_spaces(p.line);

						const char* delimiters = " \r\n";

						bool res = true;

						i32 value;
						if (line_read_i32(p.line, value, delimiters, (u32)strlen(delimiters))) {

							if (value < 0 || value > 10)
								res = false;
							else {

								switch (value) {

								case 0: // Color on and Ambient off
									mat->transparent = false;
									mat->culling = RasterizerCullMode_Back;
									break;
				    
								case 1: // Color on and Ambient on
									mat->transparent = false;
									mat->culling = RasterizerCullMode_Back;
									break;
				    
								case 2: // Highlight on
									mat->transparent = false;
									mat->culling = RasterizerCullMode_Back;
									break;
				    
								case 3: // Reflection on and Ray trace on
									mat->transparent = false;
									mat->culling = RasterizerCullMode_Back;
									break;
				    
								case 4: // Transparency: Glass on. Reflection: Ray trace on
									mat->transparent = true;
									mat->culling = RasterizerCullMode_None;
									break;
				    
								case 5:	// Reflection: Fresnel on and Ray trace on
									mat->transparent = false;
									mat->culling = RasterizerCullMode_Back;
									break;
				    
								case 6: // Transparency: Refraction on. Reflection: Fresnel off and Ray trace on
									mat->transparent = true;
									mat->culling = RasterizerCullMode_None;
									break;
				    
								case 7: // Transparency: Refraction on. Reflection: Fresnel on and Ray trace on
									mat->transparent = true;
									mat->culling = RasterizerCullMode_None;
									break;
				    
								case 8: // Reflection on and Ray trace off
									mat->transparent = false;
									mat->culling = RasterizerCullMode_Back;
									break;
				    
								case 9:	// Transparency: Glass on. Reflection: Ray trace off
									mat->transparent = true;
									mat->culling = RasterizerCullMode_None;
									break;
				    
								case 10:// Casts shadows onto invisible surfaces
									mat->transparent = false;
									mat->culling = RasterizerCullMode_Back;
									break;
				    
								}
							}
						}
						else res = false;
			
						if (!res) {

							SV_LOG_ERROR("Illum value not available in mtl '%s', line %u", filepath, p.line_count);
						}
					}
				}
				break;

				case 'n':
				{
					++p.line;
					if (p.line[0] == 'e' && p.line[1] == 'w' && p.line[2] == 'm' && p.line[3] == 't' && p.line[4] == 'l') {

						p.line += 5;
						line_jump_spaces(p.line);

						char delimiters[] = {
							' ',
							'\n',
							'\r'
						};
		    
						size_t name_size = string_split(p.line, delimiters, 3u);
						if (name_size == 0u) {
			
							SV_LOG_ERROR("Can't read the mtl name in line %u", p.line_count);
							corrupted = true;
						}
						else {

							if (use_default) {

								use_default = false;
							}
							else {
								mat = &model_info.materials.emplace_back();
							}

							mat->name.set(p.line, 0u, name_size);
							set_default_material(mat);
						}
					}
				}
				break;

				case 'K':
				{
					++p.line;
					switch (*p.line++) {

					case 'a':
					{
						v3_f32 color;

						if (line_read_v3_f32(p.line, color)) {

							mat->ambient_color = color_float(color.x, color.y, color.z);
						}
						else {
							corrupted = true;
							SV_LOG_ERROR("Can't read the mtl ambient color in line %u", p.line_count);
						}
					}
					break;
		    
					case 'd':
					{
						v3_f32 color;

						if (line_read_v3_f32(p.line, color)) {

							mat->diffuse_color = color_float(color.x, color.y, color.z);
						}
						else {
							corrupted = true;
							SV_LOG_ERROR("Can't read the mtl diffuse color in line %u", p.line_count);
						}   
					}
					break;
		    
					case 's':
					{
						v3_f32 color;

						if (line_read_v3_f32(p.line, color)) {

							mat->specular_color = color_float(color.x, color.y, color.z);
						}
						else {
							corrupted = true;
							SV_LOG_ERROR("Can't read the mtl specular color in line %u", p.line_count);
						}
					}
					break;
		    
					case 'e':
					{
						v3_f32 color;

						if (line_read_v3_f32(p.line, color)) {

							mat->emissive_color = color_float(color.x, color.y, color.z);
						}
						else {
							corrupted = true;
							SV_LOG_ERROR("Can't read the mtl emissive color in line %u", p.line_count);
						}
					}
					break;

					default:
						corrupted = true;
			
					}
				}
				break;

				case 'N':
				{
					++p.line;

					switch (*p.line++) {
			
					case 's': // Shininess
					{
						line_read_f32(p.line, mat->shininess);
					}
					break;

					case 'i': // TODO: index of refraction
					{
			
					}
					break;
			
					}		    
				}
				break;
				}	
			}

			for (const MaterialInfo& m : model_info.materials) {

				SV_LOG("Material '%s'", m.name.c_str());
				SV_LOG("Ambient %u / %u / %u", m.ambient_color.r, m.ambient_color.g, m.ambient_color.b);
				SV_LOG("Diffuse %u / %u / %u", m.diffuse_color.r, m.diffuse_color.g, m.diffuse_color.b);
				SV_LOG("Specular %u / %u / %u", m.specular_color.r, m.specular_color.g, m.specular_color.b);
				SV_LOG("Emissive %u / %u / %u", m.emissive_color.r, m.emissive_color.g, m.emissive_color.b);
				if (m.diffuse_map_path.c_str()) SV_LOG("Diffuse map %s", m.diffuse_map_path.c_str());
				if (m.normal_map_path.c_str()) SV_LOG("Normal map %s", m.normal_map_path.c_str());
			}
		}
		else SV_LOG_ERROR("Can't load the mtl file at '%s'", filepath);
    }

    SV_AUX u32 parse_objindex_to_absolute(i32 i, i32 max)
    {
		if (i > 0) {
			--i;
		}
		else if (i < 0) {
			i = max + i;
		}
		else i = 0;

		return u32(i);
    }

    bool load_model(const char* filepath, ModelInfo& model_info)
    {
		const char* model_extension = filepath_extension(filepath);

		if (model_extension == nullptr) {
			SV_LOG_ERROR("The filepath '%s' isn't a valid model filepath", filepath);
			return false;
		}

		if (strcmp(model_extension, ".obj") != 0) {
			SV_LOG_ERROR("Model format '%s' not supported", model_extension);
			return false;
		}

		// Get folder path
		{
			const char* name = filepath_name(filepath);
			size_t folderpath_size = name - filepath;
			memcpy(model_info.folderpath, filepath, folderpath_size);
			model_info.folderpath[folderpath_size] = '\0';
		}

		// Load .obj file
		String file;
	
		if (file_read_text(filepath, file)) {

			struct ObjTriangle {
				i32 position_indices[3u];
				i32 normal_indices[3u];
				i32 texcoord_indices[3u];
				bool smooth;
			};
	    
			struct ObjMesh {
				String name;
				List<ObjTriangle> triangles;
				u32 material_index;
			};
	    
			List<v3_f32> positions;
			List<v3_f32> normals;
			List<v2_f32> texcoords;
			List<ObjMesh> meshes;
	    
			ObjMesh* mesh = &meshes.emplace_back();
			mesh->material_index = u32_max;
			bool using_default = true;
			bool smooth = true;
	    
			LineProcessor p;
			line_begin(p, file.c_str());
	    
			bool corrupted = false;

			while (line_next(p) && !corrupted) {

				line_jump_spaces(p.line);

				switch (*p.line) {

				case '#': // Comment
					break;

				case 'u': // use mtl
				{
					++p.line;
					if (p.line[0] == 's' && p.line[1] == 'e' && p.line[2] == 'm' && p.line[3] == 't' && p.line[4] == 'l') {

						p.line += 5u;
						line_jump_spaces(p.line);
			
						char delimiters[] = {
							' ',
							'\n',
							'\r'
						};
		    
						size_t name_size = string_split(p.line, delimiters, 3u);
						if (name_size == 0u) {
			
							SV_LOG_ERROR("Can't read the mtl name in line %u", p.line_count);
							corrupted = true;
						}
						else {

							char texpath[FILEPATH_SIZE + 1u];
							memcpy(texpath, p.line, name_size);
							texpath[name_size] = '\0';

							u32 index = u32_max;
			    
							foreach(i, model_info.materials.size()) {

								const MaterialInfo& m = model_info.materials[i];
				
								if (strcmp(m.name.c_str(), texpath) == 0) {
									index = i;
									break;
								}
							}

							if (index == u32_max) {

								SV_LOG_ERROR("Material %s not found, line %u", texpath, p.line_count);
								corrupted = true;
							}
							else {

								mesh->material_index = index;
							}
						}
					}
				}
				break;
		    
				case 'm': // .mtl path
				{
					++p.line;
					if (p.line[0] == 't' && p.line[1] == 'l' && p.line[2] == 'l' && p.line[3] == 'i' && p.line[4] == 'b') {

						p.line += 5u;
						line_jump_spaces(p.line);
			
						char delimiters[] = {
							' ',
							'\n',
							'\r'
						};
		    
						size_t name_size = string_split(p.line, delimiters, 3u);
						if (name_size == 0u) {
			
							SV_LOG_ERROR("Can't read the mtl path in line %u", p.line_count);
							corrupted = true;
						}
						else {

							char mtlname[FILEPATH_SIZE + 1u] = "";
							memcpy(mtlname, p.line, name_size);
							mtlname[name_size] = '\0';

							char mtlpath[FILEPATH_SIZE + 1u] = "";
							strcat(mtlpath, filepath);

							size_t s = strlen(mtlpath) - 1u;

							while (s && mtlpath[s] != '/') --s;
							mtlpath[s + 1u] = '\0';
							strcat(mtlpath, mtlname);

							read_mtl(mtlpath, model_info);
						}
					}
				}
				break;

				case 'o': // New Object
				{
					++p.line;

					line_jump_spaces(p.line);

					char delimiters[] = {
						' ',
						'\n',
						'\r'
					};
		    
					size_t name_size = string_split(p.line, delimiters, 3u);
					if (name_size == 0u) {

						SV_LOG_ERROR("Can't read the object name in line %u", p.line_count);
						corrupted = true;
					}
					else {

						if (using_default) {
							using_default = false;
						}
						else {

							mesh = &meshes.emplace_back();
							mesh->material_index = u32_max;
						}

						mesh->name.set(p.line, 0u, name_size);
					}
				}
				break;

				case 's':
				{
					++p.line;
					line_jump_spaces(p.line);
					if (*p.line != 1)
						smooth = false;
					else smooth = true;
				}
				break;

				case 'g': // New Group
				{
					++p.line;

					line_jump_spaces(p.line);

					char delimiters[] = {
						' ',
						'\n',
						'\r'
					};
		    
					size_t name_size = string_split(p.line, delimiters, 3u);
					if (name_size == 0u) {

						SV_LOG_ERROR("Can't read the object name in line %u", p.line_count);
						corrupted = true;
					}
					else {

						if (mesh->triangles.empty()) {

							mesh->name.set(p.line, 0u, name_size);
						}
						else {

							mesh = &meshes.emplace_back();
							mesh->material_index = u32_max;
						}
					}
				}
				break;

				case 'v': // Vertex info
				{
					++p.line;

					switch(*p.line) {

					case ' ': // Position
					{
						v3_f32& v = positions.emplace_back();

						bool res = line_read_f32(p.line, v.x);
						if (res) res = line_read_f32(p.line, v.y);
						if (res) res = line_read_f32(p.line, v.z);
			
						if (!res) {
							corrupted = true;
							SV_LOG_ERROR("Can't read the vector at line %u", p.line_count);
						}
					}
					break;

					case 'n': // Normal
					{
						++p.line;
			
						v3_f32& v = normals.emplace_back();

						bool res = line_read_f32(p.line, v.x);
						if (res) res = line_read_f32(p.line, v.y);
						if (res) res = line_read_f32(p.line, v.z);
			
						if (!res) {
							corrupted = true;
							SV_LOG_ERROR("Can't read the vector at line %u", p.line_count);
						}
					}
					break;

					case 't': // Texcoord
					{
						++p.line;

						v2_f32& v = texcoords.emplace_back();

						if (!line_read_f32(p.line, v.x)) {
							corrupted = true;
							SV_LOG_ERROR("Can't read the vector at line %u", p.line_count);
						}

						// v coord is optional
						line_read_f32(p.line, v.y);

						v.y = 1.f - v.y;

					}
					break;
		    
					}		    
				}
				break;

				case 'f': // Face
				{
					++p.line;
					line_jump_spaces(p.line);

					u32 vertex_count = 0u;

					constexpr u32 POLYGON_COUNT = 4u;
		    
					i32 position_index[POLYGON_COUNT] = {};
					i32 normal_index[POLYGON_COUNT] = {};
					i32 texcoord_index[POLYGON_COUNT] = {};

					char delimiters[] = {
						' ',
						'/'
					};

					bool res;

					foreach(i, POLYGON_COUNT) {

						res = line_read_i32(p.line, position_index[i], delimiters, 2u);
						if (res && *p.line == '/') {

							++p.line;

							if (*p.line == '/') {
								res = line_read_i32(p.line, normal_index[i], delimiters, 2u);
							}
							else {
								res = line_read_i32(p.line, texcoord_index[i], delimiters, 2u);

								if (res && *p.line == '/') {
									++p.line;
									res = line_read_i32(p.line, normal_index[i], delimiters, 2u);
								}
							}
						}

						if (!res) break;

						++vertex_count;

						line_jump_spaces(p.line);
						if (*p.line == '\r' || *p.line == '\n') break;
					}

					if (*p.line != '\r' && *p.line != '\n') {
						SV_LOG_ERROR("Can't read more than %u vertices in one face. Try to triangulate this .obj. Line %u", POLYGON_COUNT, p.line_count);
					}

					if (!res || vertex_count < 3) {
						res = false;
						SV_LOG_ERROR("Can't read the face at line %u", p.line_count);
						corrupted = false;
					}

					if (res) {
			    
						if (vertex_count == 3u) {

							ObjTriangle& t = mesh->triangles.emplace_back();
							t.smooth = smooth;
			    
							foreach(i, 3u) {
				
								t.position_indices[i] = position_index[i];
								t.normal_indices[i] = normal_index[i];
								t.texcoord_indices[i] = texcoord_index[i];
							}
						}
						else if (vertex_count == 4u) {

							ObjTriangle& t0 = mesh->triangles.emplace_back();
							t0.smooth = smooth;

							foreach(i, 3u) {

								t0.position_indices[i] = position_index[i];
								t0.normal_indices[i] = normal_index[i];
								t0.texcoord_indices[i] = texcoord_index[i];
							}

							ObjTriangle& t1 = mesh->triangles.emplace_back();
							t1.smooth = smooth;

							foreach(j, 3u) {

								u32 i;
				    
								switch (j) {

								case 0:
									i = 0u;
									break;

								case 1:
									i = 2u;
									break;

								default:
									i = 3u;
									break;
					
								}
				
								t1.position_indices[j] = position_index[i];
								t1.normal_indices[j] = normal_index[i];
								t1.texcoord_indices[j] = texcoord_index[i];
							}
						}
					}			
				}
				break;
		
				}
			}

			if (corrupted) {
				return false;
			}

			// Parse obj format to my own format
			{
				for (ObjMesh& obj_mesh : meshes) {

					if (obj_mesh.triangles.empty())
						continue;

					// Compute min and max position indices
					u32 min_position_index = u32_max;
					u32 max_position_index = 0u;
					for (const ObjTriangle& t : obj_mesh.triangles) {

						foreach(j, 3u) {
			    
							i32 i = t.position_indices[j];
							SV_ASSERT(i != 0);

							if (i > 0) {
								--i;
							}
							else if (i < 0) {
								i = i32(positions.size()) + i;
							}
							else i = min_position_index;

							min_position_index = SV_MIN(u32(i), min_position_index);
							max_position_index = SV_MAX(u32(i), max_position_index);
						}
					}

					MeshInfo& mesh = model_info.meshes.emplace_back();

					if (obj_mesh.name.size())
						mesh.name.set(obj_mesh.name.c_str());

					mesh.material_index = obj_mesh.material_index;

					u32 elements = max_position_index - min_position_index + 1u;

					mesh.positions.resize(elements);
					mesh.normals.resize(elements);
					mesh.texcoords.resize(elements);

					mesh.indices.resize(obj_mesh.triangles.size() * 3u);

					memcpy(mesh.positions.data(), positions.data() + min_position_index, elements * sizeof(v3_f32));

					const ObjTriangle* it0 = obj_mesh.triangles.data();
					MeshIndex* it1 = mesh.indices.data();
					const ObjTriangle* end = obj_mesh.triangles.data() + obj_mesh.triangles.size();

					i32 max = i32(positions.size());
					u32 min = min_position_index;

					while (it0 != end) {

						foreach(j, 3u) {
			    
							u32 index = parse_objindex_to_absolute(it0->position_indices[j], max) - min;
							u32 normal_index = parse_objindex_to_absolute(it0->normal_indices[j], max);
							u32 texcoord_index = parse_objindex_to_absolute(it0->texcoord_indices[j], max);

							// TODO: Handle errors
			
							mesh.normals[index] = normals[normal_index];
							mesh.texcoords[index] = texcoords[texcoord_index];
			
							*it1 = index;
							++it1;
						}

						++it0;
					}
				}
			}

			// Centralize meshes
			for (MeshInfo& mesh : model_info.meshes) {

				f32 min_x = f32_max;
				f32 min_y = f32_max;
				f32 min_z = f32_max;
				f32 max_x = -f32_max;
				f32 max_y = -f32_max;
				f32 max_z = -f32_max;

				for (const v3_f32& pos : mesh.positions) {

					min_x = SV_MIN(min_x, pos.x);
					min_y = SV_MIN(min_y, pos.y);
					min_z = SV_MIN(min_z, pos.z);

					max_x = SV_MAX(max_x, pos.x);
					max_y = SV_MAX(max_y, pos.y);
					max_z = SV_MAX(max_z, pos.z);
				}

				v3_f32 center = v3_f32(min_x + (max_x - min_x) * 0.5f, min_y + (max_y - min_y) * 0.5f, min_z + (max_z - min_z) * 0.5f);
				f32 dim = SV_MAX(SV_MAX(max_x - min_x, max_y - min_y), max_z - min_z);

				for (v3_f32& pos : mesh.positions) {

					pos -= center;
					pos /= dim;
				}

				mesh.transform_matrix = XMMatrixScaling(dim, dim, dim) * XMMatrixTranslation(center.x, center.y, center.z);
			}
		}
		else {
			SV_LOG_ERROR("Can't load the model '%s', not found", filepath);
			return false;
		}

		return true;
    }

    SV_AUX void import_texture(Serializer& s, const char* realpath, const char* folderpath, const char* srcpath)
    {
		if (realpath == nullptr) {
			serialize_string(s, "");
			return;
		}
	
		const char* texname = filepath_name(realpath);
		char texpath[FILEPATH_SIZE + 1u];
		strcpy(texpath, folderpath);
		strcat(texpath, "images/");
		strcat(texpath, texname);

		char srctexpath[FILEPATH_SIZE + 1u];
		strcpy(srctexpath, srcpath);
		strcat(srctexpath, realpath);
	
		if (!file_exists(texpath)) {
	    
			if (!file_copy(srctexpath, texpath)) {
				SV_LOG_ERROR("Can't copy the file from '%s' to '%s'", srctexpath, texpath);
			}
			else SV_LOG_INFO("Texture imported '%s'", texpath);
		}
	
		serialize_string(s, texpath);
    }

    bool import_model(const char* filepath, const ModelInfo& model_info)
    {
		Serializer s;

		// Preparing folder path
	
		size_t folderpath_size = strlen(filepath);
		if (folderpath_size >= FILEPATH_SIZE - 3u) {
			SV_LOG_ERROR("This filepath is too large '%s'", filepath);
			return false;
		}

		char folderpath[FILEPATH_SIZE + 1u] = "";
		if (folderpath_size) {

			if (folderpath[folderpath_size - 1u] != '/') {
				sprintf(folderpath, "%s/", filepath);
			}
			else strcpy(folderpath, filepath);
		}

		folderpath_size = strlen(folderpath);
		if (folderpath_size >= FILEPATH_SIZE) {
			SV_LOG_ERROR("This filepath is too large '%s'", filepath);
			return false;
		}

		// Creating mesh files
	
		for (const MeshInfo& mesh : model_info.meshes) {

			if (!mesh.import)
				continue;

			size_t meshpath_size = folderpath_size + mesh.name.size() + strlen(".mesh");
			if (meshpath_size >= FILEPATH_SIZE) {
				SV_LOG_ERROR("This filepath is too large '%s'", filepath);
				return false;
			}
	    
			serialize_begin(s);

			serialize_u32(s, 1u); // VERSION

			serialize_v3_f32_array(s, mesh.positions);
			serialize_v3_f32_array(s, mesh.normals);
			serialize_v2_f32_array(s, mesh.texcoords);
			serialize_u32_array(s, mesh.indices);

			if (mesh.material_index == u32_max)
				serialize_string(s, "");
			else
				serialize_string(s, model_info.materials[mesh.material_index].name);

			serialize_xmmatrix(s, mesh.transform_matrix);

			char meshpath[FILEPATH_SIZE + 1u];
			sprintf(meshpath, "%s%s.mesh", folderpath, mesh.name.c_str());

			if (!serialize_end(s, meshpath)) {
				SV_LOG_ERROR("Can't save the mesh '%s'", meshpath);
				return false;
			}
			else {
				SV_LOG_INFO("Mesh imported '%s'", meshpath);
			}
		}

		// Creating material files

		for (const MaterialInfo& mat : model_info.materials) {

			if (!mat.import)
				continue;

			size_t matpath_size = folderpath_size + mat.name.size() + strlen(".mat");
			if (matpath_size >= FILEPATH_SIZE) {
				SV_LOG_ERROR("This filepath is too large '%s'", filepath);
				return false;
			}
	    
			serialize_begin(s);

			serialize_u32(s, 1u); // VERSION

			// Renderer settings
			{
				serialize_bool(s, mat.transparent);
				serialize_u32(s, mat.culling);
			}

			serialize_color(s, mat.ambient_color);
			serialize_color(s, mat.diffuse_color);
			serialize_color(s, mat.specular_color);
			serialize_color(s, mat.emissive_color);
			serialize_f32(s, mat.shininess);

			import_texture(s, mat.diffuse_map_path.c_str(), folderpath, model_info.folderpath);
			import_texture(s, mat.normal_map_path.c_str(), folderpath, model_info.folderpath);
			import_texture(s, mat.specular_map_path.c_str(), folderpath, model_info.folderpath);
			import_texture(s, mat.emissive_map_path.c_str(), folderpath, model_info.folderpath);

			char matpath[FILEPATH_SIZE + 1u];
			sprintf(matpath, "%s%s.mat", folderpath, mat.name.c_str());

			if (!serialize_end(s, matpath)) {
				SV_LOG_ERROR("Can't save the material '%s'", matpath);
				return false;
			}
			else {
				SV_LOG_INFO("Material imported '%s'", matpath);
			}	    
		}
	
		return true;
    }

	bool save_mesh(const Mesh& mesh, const char* filepath)
	{
		Serializer s;
		serialize_begin(s);

		serialize_u32(s, 1u); // VERSION

		serialize_v3_f32_array(s, mesh.positions);
		serialize_v3_f32_array(s, mesh.normals);
		serialize_v2_f32_array(s, mesh.texcoords);
		serialize_u32_array(s, mesh.indices);

		serialize_string(s, mesh.model_material_filepath);

		serialize_xmmatrix(s, mesh.model_transform_matrix);

		if (!serialize_end(s, filepath)) {
			SV_LOG_ERROR("Can't save the mesh '%s'", filepath);
			return false;
		}
		else SV_LOG_INFO("Mesh saved at '%s' filepath");

		return true;
	}
	
    bool save_material(const Material& mat, const char* filepath)
	{
		Serializer s;
		serialize_begin(s);

		serialize_u32(s, 1u); // VERSION

		// Renderer settings
		{
			serialize_bool(s, mat.transparent);
			serialize_u32(s, mat.culling);
		}

		serialize_color(s, mat.ambient_color);
		serialize_color(s, mat.diffuse_color);
		serialize_color(s, mat.specular_color);
		serialize_color(s, mat.emissive_color);
		serialize_f32(s, mat.shininess);

		serialize_string(s, string_validate(mat.diffuse_map.get_filepath()));
		serialize_string(s, string_validate(mat.normal_map.get_filepath()));
		serialize_string(s, string_validate(mat.specular_map.get_filepath()));
		serialize_string(s, string_validate(mat.emissive_map.get_filepath()));

		if (!serialize_end(s, filepath)) {
			SV_LOG_ERROR("Can't save the material '%s'", filepath);
			return false;
		}
		else {
			SV_LOG_INFO("Material saved at '%s'", filepath);
		}
		return true;
	}

    bool load_mesh(Mesh& mesh, const char* filepath)
    {
		Deserializer d;

		if (deserialize_begin(d, filepath)) {

			u32 version;
			deserialize_u32(d, version);
	    
			deserialize_v3_f32_array(d, mesh.positions);
			deserialize_v3_f32_array(d, mesh.normals);
			deserialize_v2_f32_array(d, mesh.texcoords);
			deserialize_u32_array(d, mesh.indices);

			char matname[FILEPATH_SIZE + 1u];
			deserialize_string(d, matname, FILEPATH_SIZE + 1u);

			if (strlen(filepath)) {

				strcat(matname, ".mat");
				char matpath[FILEPATH_SIZE + 1u];

				size_t s = strlen(filepath) - 1u;
				while (s && filepath[s] != '/') --s;

				if (s) s++;
		
				memcpy(matpath, filepath, s);
				matpath[s] = '\0';
		
				strcat(matpath, matname);

				string_copy(mesh.model_material_filepath, matpath, FILEPATH_SIZE + 1u);
			}

			if (version != 0) {
				deserialize_xmmatrix(d, mesh.model_transform_matrix);
			}
	    
			deserialize_end(d);
		}
		else {
			SV_LOG_ERROR("Mesh file '%s', not found", filepath);
			return false;
		}

		mesh_calculate_tangents(mesh);
	
		return true;
    }

    bool load_material(Material& mat, const char* filepath)
    {
		Deserializer d;

		if (deserialize_begin(d, filepath)) {

			u32 version;
			deserialize_u32(d, version);

			if (version != 0u) {
				deserialize_bool(d, mat.transparent);
				deserialize_u32(d, (u32&)mat.culling);
			}

			deserialize_color(d, mat.ambient_color);
			deserialize_color(d, mat.diffuse_color);
			deserialize_color(d, mat.specular_color);
			deserialize_color(d, mat.emissive_color);
			deserialize_f32(d, mat.shininess);

			constexpr size_t buff_size = FILEPATH_SIZE + 1u;
			char texpath[buff_size];

			// TODO: Check errors
			deserialize_string(d, texpath, buff_size);
			if (texpath[0])
				load_asset_from_file(mat.diffuse_map, texpath);
	    
			deserialize_string(d, texpath, buff_size);
			if (texpath[0])
				load_asset_from_file(mat.normal_map, texpath);

			deserialize_string(d, texpath, buff_size);
			if (texpath[0])
				load_asset_from_file(mat.specular_map, texpath);

			deserialize_string(d, texpath, buff_size);
			if (texpath[0])
				load_asset_from_file(mat.emissive_map, texpath);
	    
			deserialize_end(d);
		}
		else {
			SV_LOG_ERROR("Mesh file '%s', not found", filepath);
			return false;
		}
	
		return true;
    }

}
