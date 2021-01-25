#include "SilverEngine/core.h"

#include "SilverEngine/mesh.h"

#define ASSERT_VERTICES() SV_ASSERT(positions.size() == normals.size())

namespace sv {

	/*
	Vertices: 4
	Indices: 6
	*/
	SV_INLINE static void computePlane(Mesh& mesh, const XMMATRIX& transformMatrix, const XMVECTOR& rotationQuaternion, size_t vertexOffset, size_t indexOffset)
	{
		XMVECTOR v0, v1, v2, v3;

		// Positions
		v0 = XMVector3Transform(XMVectorSet(-0.5f, 0.f, -0.5f, 0.f), transformMatrix);
		v1 = XMVector3Transform(XMVectorSet(0.5f, 0.f, -0.5f, 0.f), transformMatrix);
		v2 = XMVector3Transform(XMVectorSet(-0.5f, 0.f, 0.5f, 0.f), transformMatrix);
		v3 = XMVector3Transform(XMVectorSet(0.5f, 0.f, 0.5f, 0.f), transformMatrix);

		v3_f32* pos = mesh.positions.data() + vertexOffset;
		pos->setDX(v0); ++pos;
		pos->setDX(v1); ++pos;
		pos->setDX(v2); ++pos;
		pos->setDX(v3);

		// Normals
		v0 = XMVector3Rotate(XMVectorSet(0.f, -1.f, 0.f, 0.f), rotationQuaternion);

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

	void Mesh::applyPlane(const XMMATRIX& transformMatrix)
	{
		ASSERT_VERTICES();

		size_t vertexOffset = positions.size();
		size_t indexOffset = indices.size();

		positions.resize(vertexOffset + 4u);
		normals.resize(vertexOffset + 4u);

		indices.resize(indexOffset + 6u);

		computePlane(*this, transformMatrix, XMQuaternionRotationMatrix(transformMatrix), vertexOffset, indexOffset);
	}

	void Mesh::applyCube(const XMMATRIX& transformMatrix)
	{
		ASSERT_VERTICES();

		size_t vertexOffset = positions.size();
		size_t indexOffset = indices.size();

		positions.resize(vertexOffset + 6u * 4u);
		normals.resize(vertexOffset + 6u * 4u);

		indices.resize(indexOffset + 6u * 6u);

		XMMATRIX faceTransform, faceRotation;
		XMVECTOR quat;

		// UP
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * transformMatrix;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(*this, faceTransform, quat, vertexOffset + 4u * 0u, indexOffset + 6 * 0u);

		// DOWN
		faceRotation = XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(180.f));
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transformMatrix;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(*this, faceTransform, quat, vertexOffset + 4u * 1u, indexOffset + 6 * 1u);

		// LEFT
		faceRotation = XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(-90.f));
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transformMatrix;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(*this, faceTransform, quat, vertexOffset + 4u * 2u, indexOffset + 6 * 2u);

		// RIGHT
		faceRotation = XMMatrixRotationRollPitchYaw(0.f, 0.f, ToRadians(90.f));
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transformMatrix;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(*this, faceTransform, quat, vertexOffset + 4u * 3u, indexOffset + 6 * 3u);

		// FRONT
		faceRotation = XMMatrixRotationRollPitchYaw(ToRadians(-90.f), 0.f, 0.f);
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transformMatrix;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(*this, faceTransform, quat, vertexOffset + 4u * 4u, indexOffset + 6 * 4u);

		// BACK
		faceRotation = XMMatrixRotationRollPitchYaw(ToRadians(90.f), 0.f, 0.f);
		faceTransform = XMMatrixTranslation(0.f, 0.5f, 0.f) * faceRotation * transformMatrix;
		quat = XMQuaternionRotationMatrix(faceTransform);
		computePlane(*this, faceTransform, quat, vertexOffset + 4u * 5u, indexOffset + 6 * 5u);

	}

	void Mesh::applySphere(const XMMATRIX& transform)
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

	Result Mesh::createGPUBuffers(ResourceUsage usage)
	{
		ASSERT_VERTICES();
		SV_ASSERT(usage != ResourceUsage_Staging);
		if (vertexBuffer || indexBuffer) return Result_Duplicated;

		std::vector<MeshVertex> vertexData;
		constructVertexData(*this, vertexData);

		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Vertex;
		desc.usage = usage;
		desc.CPUAccess = (usage == ResourceUsage_Static) ? CPUAccess_None : CPUAccess_Write;
		desc.size = u32(vertexData.size() * sizeof(MeshVertex));
		desc.pData = vertexData.data();

		svCheck(graphics_buffer_create(&desc, &vertexBuffer));

		desc.indexType = IndexType_32;
		desc.bufferType = GPUBufferType_Index;
		desc.size = u32(indices.size() * sizeof(u32));
		desc.pData = indices.data();

		svCheck(graphics_buffer_create(&desc, &indexBuffer));

		graphics_name_set(vertexBuffer, "MeshVertexBuffer");
		graphics_name_set(indexBuffer, "MeshIndexBuffer");

		return Result_Success;
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

	// DRAW CALLS

	void draw_mesh(const Mesh* mesh, const MeshMaterial* material, const XMMATRIX& transform_matrix, GPUImage* offscreen, CommandList cmd)
	{

	}

}