#include "core.h"

#include "model_internal.h"

namespace sv {

	void Mesh::computeCube(const vec2f& offset, const vec2f& size)
	{
		SV_LOG_ERROR("TODO");
	}

	void Mesh::computeSphere(const vec2f& offset, float radius)
	{
		SV_LOG_ERROR("TODO");
	}

	void Mesh::optimize()
	{
		SV_LOG_ERROR("TODO");
	}

	void Mesh::recalculateNormals()
	{
		SV_LOG_ERROR("TODO");
	}

	Result Mesh::createGPUBuffers()
	{
		SV_LOG_ERROR("TODO");
		return Result_TODO;
	}

	Result Mesh::updateGPUBuffers(CommandList cmd)
	{
		SV_LOG_ERROR("TODO");
		return Result_TODO;
	}

	Result Mesh::clear()
	{
		positions.clear();
		normals.clear();

		indices.clear();

		svCheck(graphics_destroy(vertexBuffer));
		svCheck(graphics_destroy(indexBuffer));
		return Result_Success;
	}

}