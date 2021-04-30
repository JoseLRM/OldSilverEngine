#include "core/mesh.h"

#include <sstream>

#define ASSERT_VERTICES() SV_ASSERT(mesh.positions.size() == mesh.normals.size() && mesh.positions.size() == mesh.tangents.size() && (mesh.texcoords.empty() || mesh.positions.size() == mesh.texcoords.size()))

namespace sv {

    /*
      Vertices: 4
      Indices: 6
    */
    SV_INLINE static void computePlane(Mesh& mesh, const XMMATRIX& transformMatrix, const XMVECTOR& rotationQuaternion, size_t vertexOffset, size_t indexOffset)
    {
	XMVECTOR v0, v1, v2, v3;

	// Positions
	v0 = XMVector3Transform(XMVectorSet(-0.5f, 0.f, 0.5f, 0.f), transformMatrix);
	v1 = XMVector3Transform(XMVectorSet(0.5f, 0.f, 0.5f, 0.f), transformMatrix);
	v2 = XMVector3Transform(XMVectorSet(-0.5f, 0.f, -0.5f, 0.f), transformMatrix);
	v3 = XMVector3Transform(XMVectorSet(0.5f, 0.f, -0.5f, 0.f), transformMatrix);

	v3_f32* pos = mesh.positions.data() + vertexOffset;
	pos->setDX(v0); ++pos;
	pos->setDX(v1); ++pos;
	pos->setDX(v2); ++pos;
	pos->setDX(v3);

	// Normals
	v0 = XMVector3Rotate(XMVectorSet(0.f, 1.f, 0.f, 0.f), rotationQuaternion);

	v3_f32* nor = mesh.normals.data() + vertexOffset;
	nor->setDX(v0); ++nor;
	nor->setDX(v0); ++nor;
	nor->setDX(v0); ++nor;
	nor->setDX(v0);

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

    SV_INLINE static void constructVertexData(Mesh& mesh, std::vector<MeshVertex>& vertices)
    {
	vertices.resize(mesh.positions.size());

	MeshVertex* it;
	MeshVertex* end;
	// POSITIONS
	{
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
	{
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
	{
	    it = vertices.data();
	    end = vertices.data() + vertices.size();

	    const v3_f32* tanIt = mesh.tangents.data();

	    while (it != end)
	    {
		it->tangents = *tanIt;

		++tanIt;
		++it;
	    }
	}
	// BITANGENTS
	{
	    it = vertices.data();
	    end = vertices.data() + vertices.size();

	    const v3_f32* biIt = mesh.bitangents.data();

	    while (it != end)
	    {
		it->bitangents = *biIt;

		++biIt;
		++it;
	    }
	}
	// TEX COORDS
	if (mesh.texcoords.size()) {
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

    void mesh_apply_sphere(Mesh& mesh, const XMMATRIX& transform)
    {
	SV_LOG_ERROR("TODO");
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
	f32 inv_dim = 1.f / std::max(std::max(dimensions.x, dimensions.y), dimensions.z);

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

    void mesh_recalculate_normals(Mesh& mesh)
    {
	SV_LOG_ERROR("TODO");
    }

    bool mesh_create_buffers(Mesh& mesh, ResourceUsage usage)
    {
	ASSERT_VERTICES();
	SV_ASSERT(usage != ResourceUsage_Staging);
	if (mesh.vbuffer || mesh.ibuffer) return false;

	std::vector<MeshVertex> vertexData;
	constructVertexData(mesh, vertexData);

	GPUBufferDesc desc;
	desc.bufferType = GPUBufferType_Vertex;
	desc.usage = usage;
	desc.CPUAccess = (usage == ResourceUsage_Static) ? CPUAccess_None : CPUAccess_Write;
	desc.size = u32(vertexData.size() * sizeof(MeshVertex));
	desc.pData = vertexData.data();

	SV_CHECK(graphics_buffer_create(&desc, &mesh.vbuffer));

	desc.indexType = IndexType_32;
	desc.bufferType = GPUBufferType_Index;
	desc.size = u32(mesh.indices.size() * sizeof(u32));
	desc.pData = mesh.indices.data();

	SV_CHECK(graphics_buffer_create(&desc, &mesh.ibuffer));

	graphics_name_set(mesh.vbuffer, "MeshVertexBuffer");
	graphics_name_set(mesh.ibuffer, "MeshIndexBuffer");

	return true;
    }

    bool mesh_update_buffers(Mesh& mesh, CommandList cmd)
    {
	SV_LOG_ERROR("TODO");
	return false;
    }

    bool mesh_clear(Mesh& mesh)
    {
	mesh.positions.clear();
	mesh.normals.clear();

	mesh.indices.clear();

	graphics_destroy(mesh.vbuffer);
	graphics_destroy(mesh.ibuffer);
	return true;
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

	// Load .obj file
	String file;
	
	if (file_read_text(filepath, file)) {

	    struct ObjAuxData {
		u32 position_offset;
		u32 normal_offset;
		u32 texcoord_offset;
	    };

	    List<v3_f32> normals;
	    List<v2_f32> texcoords;
	    List<ObjAuxData> meshes_aux;

	    ObjAuxData* mesh_aux = &meshes_aux.emplace_back();
	    mesh_aux->position_offset = 0u;
	    mesh_aux->normal_offset = 0u;
	    mesh_aux->texcoord_offset = 0u;
			    
	    MeshInfo* mesh = &model_info.meshes.emplace_back();
	    bool using_default = true;
	    
	    LineProcessor p;
	    line_begin(p, file.c_str());
	    
	    bool corrupted = false;

	    while (line_next(p) && !corrupted) {

		line_jump_spaces(p.line);

		switch (*p.line) {

		case '#': // Comment
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

			    ObjAuxData* last = mesh_aux;
			    mesh_aux = &meshes_aux.emplace_back();
			    mesh_aux->position_offset = (u32)mesh->positions.size() + last->position_offset;;
			    mesh_aux->normal_offset = (u32)normals.size() + last->normal_offset;
			    mesh_aux->texcoord_offset = (u32)texcoords.size() + last->texcoord_offset;
			    
			    mesh = &model_info.meshes.emplace_back();

			    normals.reset();
			    texcoords.reset();
			}

			mesh->name.set(p.line, 0u, name_size);
		    }
		}
		break;

		case 'v': // Vertex info
		{
		    ++p.line;

		    switch(*p.line) {

		    case ' ': // Position
		    {
			v3_f32& v = mesh->positions.emplace_back();

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

			// I think v coord is optional
			line_read_f32(p.line, v.y);
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
		    
		    i32 position_index[4u] = {};
		    i32 normal_index[4u] = {};
		    i32 texcoord_index[4u] = {};

		    char delimiters[] = {
			' ',
			'/'
		    };

		    bool res;

		    foreach(i, 4) {

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

			position_index[i] -= mesh_aux->position_offset;
			normal_index[i] -= mesh_aux->normal_offset;
			texcoord_index[i] -= mesh_aux->texcoord_offset;

			++vertex_count;

			line_jump_spaces(p.line);
			if (*p.line == '\r' || *p.line == '\n') break;
		    }

		    if (!res || vertex_count < 3) {
			SV_LOG_ERROR("Can't read the face at line %u", p.line_count);
			corrupted = false;
		    }
		    else {

			i32 position_count = (i32)mesh->positions.size();
			i32 normal_count = (i32)normals.size();
			i32 texcoord_count = (i32)texcoords.size();

			foreach(i, vertex_count) {
			    
			    bool res = position_index[i] != 0 && (abs(position_index[i])) <= position_count;
			    if (res) res = normal_index[i] == 0 || (abs(normal_index[i])) <= normal_count;
			    if (res) res = texcoord_index[i] == 0 || (abs(texcoord_index[i])) <= texcoord_count;

			    if (res) {

				i32 aux = position_index[i];
				position_index[i] = (aux < 0) ? (position_count + aux) : (aux - 1);

				aux = normal_index[i];
				normal_index[i] = (aux < 0) ? (normal_count + aux) : (SV_MAX(aux - 1, 0));

				aux = texcoord_index[i];
				texcoord_index[i] = (aux < 0) ? (texcoord_count + aux) : (SV_MAX(aux - 1, 0));
			    }
			    else {
				SV_LOG_ERROR("Can't read the face at line %u, index out of bounds", p.line_count);
				corrupted = true;
				break;
			    }
			}

			if (res) {
			    
			    u32* index = (u32*) position_index;

			    if (vertex_count == 3u) {

				u32 max_index = SV_MAX(SV_MAX(index[0], index[1]), index[2]);

				if (mesh->normals.size() <= max_index) mesh->normals.resize(max_index + 1u);
				if (mesh->texcoords.size() <= max_index) mesh->texcoords.resize(max_index + 1u);

				foreach(i, 3u) {
				
				    mesh->normals[index[i]] = normals[normal_index[i]];
				    mesh->texcoords[index[i]] = texcoords[texcoord_index[i]];
				    mesh->indices.push_back(index[i]);
				}
			    }
			    // TODO: Triangulate quad faces
			}			
		    }
		}
		break;
		
		}
	    }

	    if (corrupted) {
		return false;
	    }

	    // TEMP
	    for (auto& m : model_info.meshes) {
		SV_LOG("Mesh '%s', %u positions, %u normals, %u texcoords", m.name.c_str(), m.positions.size(), m.normals.size(), m.texcoords.size());
	    }
	}
	else {
	    SV_LOG_ERROR("Can't load the model '%s', not found", filepath);
	}

	return true;
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

	    size_t meshpath_size = folderpath_size + mesh.name.size() + strlen(".mesh");
	    if (meshpath_size >= FILEPATH_SIZE) {
		SV_LOG_ERROR("This filepath is too large '%s'", filepath);
		return false;
	    }
	    
	    serialize_begin(s);

	    serialize_u32(s, 0u); // VERSION

	    serialize_v3_f32_array(s, mesh.positions);
	    serialize_v3_f32_array(s, mesh.normals);
	    serialize_v2_f32_array(s, mesh.texcoords);
	    serialize_u32_array(s, mesh.indices);

	    char meshpath[FILEPATH_SIZE + 1u];
	    sprintf(meshpath, "%s/%s.mesh", folderpath, mesh.name.c_str());

	    if (!serialize_end(s, meshpath)) {
		SV_LOG_ERROR("Can't save the mesh '%s'", meshpath);
		return false;
	    }
	    else {
		SV_LOG_INFO("Mesh file saved '%s'", meshpath);
	    }
	}

	// Creating material files

	for (const MaterialInfo& mat : model_info.materials) {

	    size_t matpath_size = folderpath_size + mat.name.size() + strlen(".mat");
	    if (matpath_size >= FILEPATH_SIZE) {
		SV_LOG_ERROR("This filepath is too large '%s'", filepath);
		return false;
	    }
	    
	    serialize_begin(s);

	    serialize_u32(s, 0u); // VERSION

	    serialize_color(s, mat.diffuse_color);
	    serialize_color(s, mat.specular_color);
	    serialize_color(s, mat.emissive_color);
	    serialize_f32(s, mat.shininess);
	    serialize_string(s, mat.diffuse_map_path);
	    serialize_string(s, mat.normal_map_path);
	    serialize_string(s, mat.specular_map_path);
	    serialize_string(s, mat.emissive_map_path);

	    char matpath[FILEPATH_SIZE + 1u];
	    sprintf(matpath, "%s/%s.mat", folderpath, mat.name.c_str());

	    if (!serialize_end(s, matpath)) {
		SV_LOG_ERROR("Can't save the material '%s'", matpath);
		return false;
	    }
	    else {
		SV_LOG_INFO("Material file saved '%s'", matpath);
	    }
	}

	// TODO: Save images
	
	return true;
    }

    bool load_mesh(const char* filepath, Mesh& mesh)
    {
	Deserializer d;

	if (deserialize_begin(d, filepath)) {

	    u32 version;
	    deserialize_u32(d, version);
	    
	    deserialize_v3_f32_array(d, mesh.positions);
	    deserialize_v3_f32_array(d, mesh.normals);
	    deserialize_v2_f32_array(d, mesh.texcoords);
	    deserialize_u32_array(d, mesh.indices);

	    // TODO: Compute tangents and bitangents
	    
	    deserialize_end(d);
	}
	else {
	    SV_LOG_ERROR("Mesh file '%s', not found", filepath);
	    return false;
	}
	
	return true;
    }

    bool load_material(const char* filepath, Material& mat)
    {
	Deserializer d;

	if (deserialize_begin(d, filepath)) {

	    u32 version;
	    deserialize_u32(d, version);
	    
	    deserialize_color(d, mat.diffuse_color);
	    deserialize_color(d, mat.specular_color);
	    deserialize_color(d, mat.emissive_color);
	    deserialize_f32(d, mat.shininess);

	    constexpr size_t buff_size = FILEPATH_SIZE + 1u;
	    char texpath[buff_size];

	    // TODO: Check errors
	    deserialize_string(d, texpath, buff_size);
	    load_asset_from_file(mat.diffuse_map, filepath);
	    
	    deserialize_string(d, texpath, buff_size);
	    load_asset_from_file(mat.normal_map, filepath);

	    deserialize_string(d, texpath, buff_size);
	    load_asset_from_file(mat.specular_map, filepath);

	    deserialize_string(d, texpath, buff_size);
	    load_asset_from_file(mat.emissive_map, filepath);
	    
	    deserialize_end(d);
	}
	else {
	    SV_LOG_ERROR("Mesh file '%s', not found", filepath);
	    return false;
	}
	
	return true;
    }

}
