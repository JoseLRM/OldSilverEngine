#include "core.h"

#include "material_system_internal.h"

namespace sv {

	static std::vector<Material_internal*>	g_MaterialsToUpdate;
	static std::mutex						g_MaterialsToUpdateMutex;

	void matsys_materials_close()
	{
		g_MaterialsToUpdate.clear();
	}

	void matsys_materials_update()
	{
		g_MaterialsToUpdateMutex.lock();
		if (!g_MaterialsToUpdate.empty()) {
			CommandList cmd = graphics_commandlist_get();
			for (Material_internal* mat : g_MaterialsToUpdate) {
				matsys_material_update(*mat, cmd);
				mat->inUpdateList = false;
			}
			g_MaterialsToUpdate.clear();
		}
		g_MaterialsToUpdateMutex.unlock();
	}

	void matsys_material_updatelist_add(Material_internal* mat)
	{
		if (!mat->inUpdateList) {
			g_MaterialsToUpdateMutex.lock();
			g_MaterialsToUpdate.push_back(mat);
			g_MaterialsToUpdateMutex.unlock();
			mat->inUpdateList = true;
		}
	}

	void matsys_material_updatelist_rmv(Material_internal* mat)
	{
		if (mat->inUpdateList) {
			mat->inUpdateList = false;
			std::lock_guard<std::mutex> lock(g_MaterialsToUpdateMutex);

			for (auto it = g_MaterialsToUpdate.begin(); it != g_MaterialsToUpdate.end(); ++it) {
				if (*(it) == mat) {
					g_MaterialsToUpdate.erase(it);
					break;
				}
			}
		}
	}

	void matsys_material_update(Material_internal& mat, CommandList cmd)
	{
		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {

			MaterialBuffer& buffer = mat.buffers[i];
			if (buffer.rawData == nullptr) continue;

			// Create buffer
			if (buffer.buffer == nullptr) {
				GPUBufferDesc desc;
				desc.bufferType = GPUBufferType_Constant;
				desc.CPUAccess = CPUAccess_Write;
				desc.usage = mat.dynamic ? ResourceUsage_Dynamic : ResourceUsage_Default;
				desc.pData = nullptr;
				desc.size = mat.shaderLibrary->bufferSizes[i];

				if (graphics_buffer_create(&desc, &buffer.buffer) != Result_Success) {
					svLogError("Can't create material buffer");
					return;
				}
			}

			// Update if necessary
			if (buffer.updateBegin != buffer.updateEnd) {
				graphics_buffer_update(buffer.buffer, buffer.rawData + buffer.updateBegin, buffer.updateEnd - buffer.updateBegin, buffer.updateBegin, cmd);
				buffer.updateBegin = 0u;
				buffer.updateEnd = 0u;
			}
		}
	}

}