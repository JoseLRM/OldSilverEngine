#include "core.h"
#include "Material.h"

namespace sv {

	bool Material::CreateBuffers(MaterialData& initialData)
	{
		// Create
		m_MaterialData = initialData;
		m_Modified = false;

		// Create Constant Buffer
		GPUBufferDesc desc;
		desc.bufferType = GPUBufferType_Constant;
		desc.usage = ResourceUsage_Default;
		desc.CPUAccess = CPUAccess_Write;
		desc.size = sizeof(MaterialData);
		desc.pData = &initialData;

		svCheck(graphics_buffer_create(&desc, m_Buffer));

		return true;
	}

	bool Material::DestroyBuffers()
	{
		svCheck(graphics_destroy(m_Buffer));
		return true;
	}

	void Material::UpdateBuffers(CommandList cmd)
	{
		if (m_Modified) {
			graphics_buffer_update(m_Buffer, &m_MaterialData, sizeof(MaterialData), 0u, cmd);
			m_Modified = false;
		}
	}

	void Material::SetMaterialData(const MaterialData& materialData)
	{
		m_MaterialData = materialData;
		m_Modified = true;
	}

	bool Material::HasValidBuffers() const
	{
		return m_Buffer.IsValid();
	}

}