#include "SilverEngine/core.h"

#include "SilverEngine/mesh.h"

#include "external/assimp/Importer.hpp"
#include "external/assimp/postprocess.h"
#include "external/assimp/scene.h"
#include "external/assimp/material.h"

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

	Result mesh_create_buffers(Mesh& mesh, ResourceUsage usage)
	{
		ASSERT_VERTICES();
		SV_ASSERT(usage != ResourceUsage_Staging);
		if (mesh.vbuffer || mesh.ibuffer) return Result_Duplicated;

		std::vector<MeshVertex> vertexData;
		constructVertexData(mesh, vertexData);

		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Vertex;
		desc.usage = usage;
		desc.CPUAccess = (usage == ResourceUsage_Static) ? CPUAccess_None : CPUAccess_Write;
		desc.size = u32(vertexData.size() * sizeof(MeshVertex));
		desc.pData = vertexData.data();

		svCheck(graphics_buffer_create(&desc, &mesh.vbuffer));

		desc.indexType = IndexType_32;
		desc.bufferType = GPUBufferType_Index;
		desc.size = u32(mesh.indices.size() * sizeof(u32));
		desc.pData = mesh.indices.data();

		svCheck(graphics_buffer_create(&desc, &mesh.ibuffer));

		graphics_name_set(mesh.vbuffer, "MeshVertexBuffer");
		graphics_name_set(mesh.ibuffer, "MeshIndexBuffer");

		return Result_Success;
	}

	Result mesh_update_buffers(Mesh& mesh, CommandList cmd)
	{
		SV_LOG_ERROR("TODO");
		return Result_TODO;
	}

	Result mesh_clear(Mesh& mesh)
	{
		mesh.positions.clear();
		mesh.normals.clear();

		mesh.indices.clear();

		svCheck(graphics_destroy(mesh.vbuffer));
		svCheck(graphics_destroy(mesh.ibuffer));
		return Result_Success;
	}

	static Result get_texture(const std::string& folderpath, std::string& str, const aiMaterial& mat, TextureAsset& tex, aiTextureType type)
	{
		// I hate you
		aiString aistr;
		mat.GetTexture(type, 0u, &aistr);

		if (aistr.length == 0u) return Result_NotFound;

		for (u32 i = 0u; i < aistr.length; ++i) {
			if (aistr.data[i] == '\\') aistr.data[i] = '/';
		}

		str = folderpath + aistr.data;

		return load_asset_from_file(tex.asset_ptr, str.c_str());
	}

	Result model_load(const char* filepath, ModelInfo& model_info)
	{
		std::string folderpath = filepath;

		SV_PARSE_FILEPATH();

		Assimp::Importer importer;

		const aiScene const* scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes);
		if (scene == nullptr || !scene->HasMeshes()) return Result_InvalidFormat;

		// Compute folder path
		{
			const char* filepath = folderpath.data();

			const char* it = filepath + strlen(filepath);
			const char* end = filepath - 1u;
			
			while (it != end) {

				if (*it == '/') {

					++it;
					size_t size = it - filepath;
					folderpath.resize(size);
					memcpy(folderpath.data(), filepath, size);
					break;
				}

				--it;
			}
		}

		model_info.meshes.resize(scene->mNumMeshes);
		model_info.materials.resize(scene->mNumMaterials);

		// Create materials
		std::string path;

		foreach(i, scene->mNumMaterials) {

			const aiMaterial& m0 = *scene->mMaterials[i];
			MaterialInfo& m1 = model_info.materials[i];

			aiColor3D color;

			m0.Get(AI_MATKEY_NAME, m1.name);
			m0.Get(AI_MATKEY_COLOR_DIFFUSE, color); m1.diffuse_color = { color.r, color.g, color.b };
			m0.Get(AI_MATKEY_COLOR_SPECULAR, color); m1.specular_color = { color.r, color.g, color.b };
			m0.Get(AI_MATKEY_SHININESS, m1.shininess);
			
			get_texture(folderpath, path, m0, m1.diffuse_map, aiTextureType_DIFFUSE);
			get_texture(folderpath, path, m0, m1.normal_map, aiTextureType_NORMALS);
			get_texture(folderpath, path, m0, m1.specular_map, aiTextureType_SPECULAR);
		}

		foreach(i, scene->mNumMeshes) {

			const aiMesh& m0 = *scene->mMeshes[i];
			MeshInfo& m1 = model_info.meshes[i];

			if (m0.mName.length) {
				m1.name = m0.mName.data;
			}

			// Material index
			m1.material_index = m0.mMaterialIndex;

			// Indices
			foreach(j, m0.mNumFaces) {

				const aiFace& face = m0.mFaces[j];
				
				SV_ASSERT(face.mNumIndices % 3u == 0u);

				u32* it = face.mIndices;
				u32* end = it + face.mNumIndices;

				while (it != end) {

					m1.indices.push_back(*it);
					++it;
				}
			}

			// Vertices
			m1.positions.resize(m0.mNumVertices);
			m1.normals.resize(m0.mNumVertices);
			m1.tangents.resize(m0.mNumVertices);
			m1.bitangents.resize(m0.mNumVertices);

			foreach(j, m0.mNumVertices) {

				aiVector3D v = m0.mVertices[j];
				m1.positions[j] = { v.x, v.y, v.z };
			}
			foreach(j, m0.mNumVertices) {

				aiVector3D v = m0.mNormals[j];
				m1.normals[j] = { v.x, v.y, v.z };
			}
			foreach(j, m0.mNumVertices) {

				aiVector3D v = m0.mTangents[j];
				m1.tangents[j] = { v.x, v.y, v.z };
			}
			foreach(j, m0.mNumVertices) {

				aiVector3D v = m0.mBitangents[j];
				m1.bitangents[j] = { v.x, v.y, v.z };
			}

			if (m0.mTextureCoords[0] != nullptr) {

				m1.texcoords.resize(m0.mNumVertices);

				foreach(j, m0.mNumVertices) {

					aiVector3D v = m0.mTextureCoords[0][j];
					m1.texcoords[j] = { v.x, 1.f - v.y };
				}
			}
		}

		return Result_Success;
	}

}
