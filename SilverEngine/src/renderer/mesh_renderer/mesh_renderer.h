#pragma once

#include "core.h"
#include "graphics.h"

#include "renderer/objects/Mesh.h"
#include "renderer/objects/Material.h"

namespace sv {

	struct MeshVertex {
		vec3 position;
		vec3 normal;
		vec2 texCoord;
	};

	struct AABB {
	};

	struct MeshInstance {
		XMMATRIX			tm;
		Mesh* pMesh;
		Material* pMaterial;
		AABB				boundingBox;

		MeshInstance() = default;
		MeshInstance(XMMATRIX m, Mesh* mesh, Material* mat) : tm(m), pMesh(mesh), pMaterial(mat) {}
	};

	bool mesh_renderer_initialize();
	bool mesh_renderer_close();
	void mesh_renderer_scene_draw_meshes(CommandList cmd);

	void mesh_renderer_mesh_render_forward(MeshInstance* instances, ui32* indices, ui32 count, bool transparent, const XMMATRIX& VM, const XMMATRIX& PM, GPUImage& renderTarget, GPUImage& depthStencil, CommandList cmd);

}