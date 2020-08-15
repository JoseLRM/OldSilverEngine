#pragma once

#include "core.h"
#include "graphics.h"

namespace sv {

	enum MeshRenderingTechnique {
		MeshRenderingTechnique_Forward,
		MeshRenderingTechnique_Deferred,
	};

	struct AABB {
	};

	typedef void* Mesh;

	bool mesh_create(ui32 vertexCount, ui32 indexCount, Mesh* pMesh);
	bool mesh_destroy(Mesh mesh);
	void mesh_positions_set(Mesh mesh, void* positions, ui32 offset, ui32 count);
	void mesh_normals_set(Mesh mesh, void* normals, ui32 offset, ui32 count);
	void mesh_texCoord_set(Mesh mesh, void* texCoord, ui32 offset, ui32 count);
	void mesh_indices_set(Mesh mesh, ui32* indices, ui32 offset, ui32 count);
	bool mesh_initialize(Mesh mesh);
	// TODO: Serialize and deserialize

	struct MaterialData {
		vec4 diffuseColor;
	};

	typedef void* Material;

	bool				material_create(MaterialData& initialData, Material* pMaterial);
	bool				material_destroy(Material material);
	void				material_data_set(Material material, const MaterialData& data);
	const MaterialData& material_data_get(Material material);

	// TODO: Serialize and deserialize

}