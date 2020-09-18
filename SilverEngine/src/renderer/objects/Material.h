#pragma once

#include "graphics.h"

namespace sv {

	struct MaterialData {
		vec4f diffuseColor;
	};

	class Material {
		MaterialData	m_MaterialData;
		GPUBuffer		m_Buffer;
		bool			m_Modified;

	public:
		bool CreateBuffers(MaterialData& initialData);
		bool DestroyBuffers();
		void UpdateBuffers(CommandList cmd);

		void SetMaterialData(const MaterialData& materialData);
		inline const MaterialData& GetMaterialData() const { return m_MaterialData; }

		inline GPUBuffer& GetConstantBuffer() { return m_Buffer; }

		bool HasValidBuffers() const;


	};

}