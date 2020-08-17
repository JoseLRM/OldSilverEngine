#include "core.h"

#include "renderer/mesh_renderer/mesh_renderer.h"
#include "Mesh.h"

namespace sv {
	bool Mesh::CreateBuffers()
	{
		if (m_VertexCount == 0u && m_IndexCount == 0u) return false;

		// Vertex Buffer
		MeshVertex* vertices = new MeshVertex[m_VertexCount];

		for (ui32 i = 0; i < m_VertexCount; ++i) {
			vertices[i].position = m_Positions[i];
		}
		for (ui32 i = 0; i < m_VertexCount; ++i) {
			vertices[i].normal = m_Normals[i];
		}
		for (ui32 i = 0; i < m_VertexCount; ++i) {
			vertices[i].texCoord = m_TexCoords[i];
		}

		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Vertex;
		desc.usage = ResourceUsage_Static;
		desc.CPUAccess = CPUAccess_None;
		desc.size = sizeof(MeshVertex) * m_VertexCount;
		desc.pData = vertices;

		svCheck(graphics_buffer_create(&desc, m_VertexBuffer));

		delete[] vertices;

		// Index Buffer
		desc.bufferType = GPUBufferType_Index;
		desc.size = sizeof(ui32) * m_IndexCount;
		desc.pData = m_Indices.data();
		desc.indexType = IndexType_32;

		svCheck(graphics_buffer_create(&desc, m_IndexBuffer));

		return true;
	}

	bool Mesh::DestroyBuffers()
	{
		graphics_destroy(m_VertexBuffer);
		graphics_destroy(m_IndexBuffer);

		return true;
	}

	void Mesh::SetVertexCount(ui32 vertexCount)
	{
		m_VertexCount = vertexCount;

		m_Positions.resize(m_VertexCount, { 0.f, 0.f, 0.f });
		m_Normals.resize(m_VertexCount, { 0.f, 1.f, 0.f });
		m_TexCoords.resize(m_VertexCount, { 0.f, 0.f });
	}
	
	void Mesh::SetIndexCount(ui32 indexCount)
	{
		m_IndexCount = indexCount;
		m_Indices.resize(m_IndexCount, 0u);
	}

	void Mesh::SetPositions(void* positions, ui32 offset, ui32 count)
	{
		SV_ASSERT(!HasValidBuffers());
		SV_ASSERT(offset + count <= m_VertexCount);

		memcpy(m_Positions.data() + offset, positions, count * sizeof(vec3));
	}

	void Mesh::SetNormals(void* normals, ui32 offset, ui32 count)
	{
		SV_ASSERT(!HasValidBuffers());
		SV_ASSERT(offset + count <= m_VertexCount);

		memcpy(m_Normals.data() + offset, normals, count * sizeof(vec3));
	}

	void Mesh::SetTexCoords(void* texCoords, ui32 offset, ui32 count)
	{
		SV_ASSERT(!HasValidBuffers());
		SV_ASSERT(offset + count <= m_VertexCount);

		memcpy(m_TexCoords.data() + offset, texCoords, count * sizeof(vec2));
	}

	void Mesh::SetIndices(ui32* indices, ui32 offset, ui32 count)
	{
		SV_ASSERT(!HasValidBuffers());
		SV_ASSERT(offset + count <= m_IndexCount);

		memcpy(m_Indices.data() + offset, indices, count * sizeof(ui32));
	}

	bool Mesh::HasValidBuffers() const
	{
		return m_VertexBuffer.IsValid() && m_IndexBuffer.IsValid();
	}
}