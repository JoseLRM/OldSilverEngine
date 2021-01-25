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

		GPUBuffer* vertexBuffer = nullptr;
		GPUBuffer* indexBuffer = nullptr;

		void applyPlane(const XMMATRIX& transform = XMMatrixIdentity());
		void applyCube(const XMMATRIX& transform = XMMatrixIdentity());
		void applySphere(const XMMATRIX& transform = XMMatrixIdentity());

		void optimize();
		void recalculateNormals();
		Result createGPUBuffers(ResourceUsage usage = ResourceUsage_Static);
		Result updateGPUBuffers(CommandList cmd);

		Result clear();

	};

	struct MeshMaterial {

	};

	void draw_mesh(const Mesh* mesh, const MeshMaterial* material, const XMMATRIX& transform_matrix, GPUImage* offscreen, CommandList cmd);

}