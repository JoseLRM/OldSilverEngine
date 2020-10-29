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

	Result matsys_material_create(Material_internal& mat, ShaderLibrary_internal* shaderLibrary, bool dynamic, bool addToUpdateList)
	{
		mat.dynamic = dynamic;
		mat.shaderLibrary = shaderLibrary;

		if (mat.shaderLibrary->bufferSizesCount) {

			// Reserve rawdata and initialize to 0
			mat.rawMemory = new ui8[mat.shaderLibrary->bufferSizesCount];
			svZeroMemory(mat.rawMemory, mat.shaderLibrary->bufferSizesCount);

			// Asign memory ptrs
			size_t offset = 0u;

			for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {

				MaterialBuffer& buf = mat.buffers[i];
				size_t size = size_t(mat.shaderLibrary->bufferSizes[i]);

				if (size) {
					buf.rawData = mat.rawMemory + offset;
					offset += size;
				}
			}
			SV_ASSERT(offset == mat.shaderLibrary->bufferSizesCount);
		}

		// Reserve and initialize to 0 texture ptrs
		mat.textures.resize(mat.shaderLibrary->texturesCount);

		// Add to update list ??
		if (addToUpdateList) matsys_material_updatelist_add(&mat);

		return Result_Success;
	}

	Result matsys_material_destroy(Material_internal& mat)
	{
		// Deallocate rawdata and destroy buffers
		{
			if (mat.rawMemory) {
				delete[] mat.rawMemory;
				mat.rawMemory = nullptr;

				MaterialBuffer* it = mat.buffers;
				MaterialBuffer* end = it + ShaderType_GraphicsCount;

				while (it != end) {
					if (it->rawData) {
						svCheck(graphics_destroy(it->buffer));
					}
					++it;
				}
			}
		}

		// Destroy shaderlibrary reference
		{
			auto& materials = mat.shaderLibrary->materials;
			for (auto it = materials.begin(); it != materials.end(); ++it) {
				if (*it._Ptr == &mat) {
					materials.erase(it);
					break;
				}
			}
		}

		// Remove from update list
		matsys_material_updatelist_rmv(&mat);

		return Result_Success;
	}

	Result matsys_material_recreate(Material_internal& mat, ShaderLibrary_internal& lib)
	{
		if (&lib == mat.shaderLibrary) return Result_Success;

		Material_internal aux;
		matsys_material_create(aux, &lib, mat.dynamic, false);

		ShaderLibrary_internal& old = *mat.shaderLibrary;

		for (ui32 i = 0u; i < old.attributes.size(); ++i) {

			ShaderLibraryAttribute& oldAtt = old.attributes[i];

			// Find if it exist
			for (ui32 j = 0u; j < lib.attributes.size(); ++j) {
				ShaderLibraryAttribute& newAtt = lib.attributes[j];

				// If exist
				if (oldAtt.type == newAtt.type && string_equal(oldAtt.name.c_str(), oldAtt.name.size(), newAtt.name.c_str(), newAtt.name.size())) {

					ShaderLibraryAttribute& att = oldAtt;

					// It's a texture or image
					if (att.type == ShaderAttributeType_Other) {

						MaterialTexture& tex = mat.textures[i];
						aux.textures[j] = std::move(tex);
						break;

					}

					// It's raw data
					else {

						ui32 typeSize = graphics_shader_attribute_size(att.type);
						XMMATRIX rawData;

						// Get the raw value
						ShaderIndices& srcIndices = old.attributeIndices[i];

						for (ui32 w = 0; w < ShaderType_GraphicsCount; ++w) {
							
							ui32 index = srcIndices.i[w];

							if (index != ui32_max) {
								memcpy(&rawData, &mat.buffers[w].rawData[index], typeSize);
								break;
							}
						}

						// Set the raw value
						ShaderIndices& dstIndices = lib.attributeIndices[j];

						for (ui32 w = 0; w < ShaderType_GraphicsCount; ++w) {

							ui32 index = dstIndices.i[w];

							if (index != ui32_max) {

								MaterialBuffer& buf = aux.buffers[w];
								memcpy(&buf.rawData[index], &rawData, typeSize);
								
								buf.updateBegin = std::min(buf.updateBegin, index);
								buf.updateEnd = std::max(buf.updateEnd, index + typeSize);
							}
						}

						break;
					}
				}
			}
		}

		matsys_material_destroy(mat);
		mat = std::move(aux);
		matsys_material_updatelist_add(&mat);

		return Result_Success;
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
					SV_LOG_ERROR("Can't create material buffer");
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