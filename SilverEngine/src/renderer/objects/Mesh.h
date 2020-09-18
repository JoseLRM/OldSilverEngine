#pragma once

#include "graphics.h"

namespace sv {

	class Mesh {
		std::vector<vec3f> m_Positions;
		std::vector<vec3f> m_Normals;
		std::vector<vec2f> m_TexCoords;

		std::vector<ui32> m_Indices;

		ui32 m_VertexCount = 0u;
		ui32 m_IndexCount = 0u;

		GPUBuffer m_VertexBuffer;
		GPUBuffer m_IndexBuffer;

	public:
		Result CreateBuffers();
		Result DestroyBuffers();

		// Setters

		void SetVertexCount(ui32 vertexCount);
		void SetIndexCount(ui32 indexCount);

		void SetPositions(void* positions, ui32 offset, ui32 count);
		void SetNormals(void* normals, ui32 offset, ui32 count);
		void SetTexCoords(void* texCoords, ui32 offset, ui32 count);
		void SetIndices(ui32* indices, ui32 offset, ui32 count);

		// Getters

		bool HasValidBuffers() const;
		inline ui32 GetVertexCount() const { return m_VertexCount; }
		inline ui32 GetIndexCount() const { return m_IndexCount; }
		inline GPUBuffer& GetVertexBuffer() { return m_VertexBuffer; }
		inline GPUBuffer& GetIndexBuffer() { return m_IndexBuffer; }

		inline const vec3f* GetPositions() const { return m_Positions.data(); }
		inline const vec3f* GetNormals() const { return m_Normals.data(); }
		inline const vec2f* GetTexCoords() const { return m_TexCoords.data(); }

		inline const ui32* GetIndices() const { return m_Indices.data(); }

	};

}