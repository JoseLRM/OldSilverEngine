#include "SilverEngine/core.h"

#include "SilverEngine/mesh.h"

#define ASSERT_VERTICES() SV_ASSERT(mesh.positions.size() == mesh.normals.size())

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

}