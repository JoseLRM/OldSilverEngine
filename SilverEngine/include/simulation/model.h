#pragma once

#include "platform/graphics.h"

namespace sv {

	typedef ui32 MeshIndex;

	struct Mesh {

		std::vector<vec3f> positions;
		std::vector<vec3f> normals;

		std::vector<MeshIndex> indices;

		GPUBuffer* vertexBuffer = nullptr;
		GPUBuffer* indexBuffer = nullptr;

		void computeCube(const vec2f& offset = { 0.f, 0.f }, const vec2f& size = { 1.f, 1.f });
		void computeSphere(const vec2f& offset = { 0.f, 0.f }, float radius = 1.f);

		void optimize();
		void recalculateNormals();
		Result createGPUBuffers();
		Result updateGPUBuffers(CommandList cmd);

		Result clear();

	};

}