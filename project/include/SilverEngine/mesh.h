#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	typedef u32 MeshIndex;

	struct MeshVertex {
		v3_f32 position;
		v3_f32 normal;
		v3_f32 tangents;
		v3_f32 bitangents;
		v2_f32 texcoord;
	};

	struct Mesh {

		std::vector<v3_f32> positions;
		std::vector<v3_f32> normals;
		std::vector<v3_f32> tangents;
		std::vector<v3_f32> bitangents;
		std::vector<v2_f32> texcoords;

		std::vector<MeshIndex> indices;

		GPUBuffer* vbuffer = nullptr;
		GPUBuffer* ibuffer = nullptr;

	};

	struct Material {
		Color3f diffuse_color;
		Color3f specular_color;
		Color3f emissive_color;
		f32		shininess;
		TextureAsset diffuse_map;
		TextureAsset normal_map;
		TextureAsset specular_map;
		TextureAsset emissive_map;
	}; 

	SV_DEFINE_ASSET(MeshAsset, Mesh);

	void mesh_apply_plane(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());
	void mesh_apply_cube(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());
	void mesh_apply_sphere(Mesh& mesh, const XMMATRIX& transform = XMMatrixIdentity());

	void mesh_set_scale(Mesh& mesh, f32 scale, bool center = false);
	void mesh_optimize(Mesh& mesh);
	void mesh_recalculate_normals(Mesh& mesh);

	Result mesh_create_buffers(Mesh& mesh, ResourceUsage usage = ResourceUsage_Static);
	Result mesh_update_buffers(Mesh& mesh, CommandList cmd);
	Result mesh_clear(Mesh& mesh);

	// Model loading

	struct MeshInfo {

		std::string name;

		std::vector<v3_f32> positions;
		std::vector<v3_f32> normals;
		std::vector<v3_f32> tangents;
		std::vector<v3_f32> bitangents;
		std::vector<v2_f32> texcoords;

		std::vector<MeshIndex> indices;

		XMMATRIX transform_matrix;
		u32 material_index = u32_max;

	};

	struct MaterialInfo {
		std::string name;
		Color3f diffuse_color;
		Color3f specular_color;
		Color3f emissive_color;
		f32 shininess;
		TextureAsset diffuse_map;
		TextureAsset normal_map;
		TextureAsset specular_map;
		TextureAsset emissive_map;
	};

	struct ModelInfo {
		std::vector<MeshInfo> meshes;
		std::vector<MaterialInfo> materials;
	};

	Result load_model(const char* filepath, ModelInfo& model_info);
	Result import_model(const char* filepath, const ModelInfo& model_info);

	Result load_mesh(const char* filepath, Mesh& mesh);

}
