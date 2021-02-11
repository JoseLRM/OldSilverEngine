#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	typedef u32 MeshIndex;

	struct MeshVertex {
		v3_f32 position;
		v3_f32 normal;
	};

	struct Mesh {

		std::vector<v3_f32> positions;
		std::vector<v3_f32> normals;

		std::vector<MeshIndex> indices;

		GPUBuffer* vbuffer = nullptr;
		GPUBuffer* ibuffer = nullptr;

	};

	struct Material {
		Color4f color;
	};

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

		std::vector<v3_f32> positions;
		std::vector<v3_f32> normals;

		std::vector<MeshIndex> indices;

		u32 material_index = u32_max;

	};

	struct MaterialInfo {
		std::string name;
		Color4f color;
	};

	struct ModelInfo {
		std::vector<MeshInfo> meshes;
		std::vector<MaterialInfo> materials;
	};

	Result model_load(const char* filepath, ModelInfo& model_info);

}