#pragma once

#include "mesh_renderer.h"
#include "graphics.h"

namespace sv {

	struct MeshVertex {
		vec3 position;
		vec3 normal;
		vec2 texCoord;
	};

	struct Mesh_internal {
		std::vector<vec3> positions;
		std::vector<vec3> normals;
		std::vector<vec2> texCoords;

		std::vector<ui32> indices;

		ui32 vertexCount;
		ui32 indexCount;

		GPUBuffer vertexBuffer;
		GPUBuffer indexBuffer;
	};

	struct Material_internal {
		MaterialData	data;
		GPUBuffer		buffer;
		bool			modified;
	};

	struct MeshInstance {
		XMMATRIX			tm;
		Mesh_internal*		pMesh;
		Material_internal*	pMaterial;
		AABB				boundingBox;

		MeshInstance() = default;
		MeshInstance(XMMATRIX m, Mesh_internal* mesh, Material_internal* mat) : tm(m), pMesh(mesh), pMaterial(mat) {}
	};

	bool mesh_renderer_initialize();
	bool mesh_renderer_close();
	void mesh_renderer_scene_draw_meshes(CommandList cmd);

	void mesh_renderer_mesh_render_forward(MeshInstance* instances, ui32* indices, ui32 count, bool transparent, const XMMATRIX& VM, const XMMATRIX& PM, GPUImage& renderTarget, GPUImage& depthStencil, CommandList cmd);

}