#include "core.h"

#include "material_system_internal.h"

namespace sv {

	std::vector<Material_internal*>	g_MaterialsToUpdate;
	std::mutex						g_MaterialsToUpdateMutex;

	Result matsys_material_create(ShaderLibrary* shaderLibrary, bool dynamic, Material** pMaterial)
	{
		SV_ASSERT(shaderLibrary);

		Material_internal mat;
		mat.create(reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary), dynamic, false);

		Material_internal* material = &g_Materials.create();
		*material = std::move(mat);
		*pMaterial = reinterpret_cast<Material*>(material);

		// Add ptr to library list
		material->shaderLibrary->matCount.fetch_add(1);

		// Add ptr to material update list
		material->addToUpdateList();

		return Result_Success;
	}

	Result matsys_material_destroy(Material* pInternal)
	{
		if (pInternal == nullptr) return Result_Success;

		PARSE_MATERIAL();

		// Notify shaderlibrary
		mat.shaderLibrary->matCount.fetch_sub(1);

		Result res = mat.destroy();
		if (result_fail(res)) {
			SV_LOG_ERROR("Can't destroy a material");
		}

		g_Materials.destroy(mat);

		return res;
	}

	void matsys_material_bind(Material* pInternal, SubShaderID subShaderID, CommandList cmd)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();
		SV_ASSERT(mat.shaderLibrary->type->subShaderCount > subShaderID);

		MaterialInfo& info = mat.shaderLibrary->matInfo;
		ShaderType shaderType = mat.shaderLibrary->type->subShaderRegisters[subShaderID].type;

		// Textures
		for (ui32 i = 0; i < info.textures.size(); ++i) {

			ui32 bindSlot = info.textureSlots[i].i[subShaderID];

			if (bindSlot != ui32_max) {

				const MaterialTexture& tex = mat.textures[i];

				if (tex.texture.hasReference()) {
					graphics_image_bind(tex.texture.get(), bindSlot, shaderType, cmd);
				}
				else graphics_image_bind(tex.image, bindSlot, shaderType, cmd);

			}
		}

		// Bind Buffer
		ui32 slot = info.bufferBindings[subShaderID];
		if (slot != ui32_max) {

			graphics_constantbuffer_bind(mat.buffers[subShaderID].buffer, slot, shaderType, cmd);
		}
	}

	void matsys_material_update(Material* pInternal, CommandList cmd)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();
		mat.update(cmd);
		mat.rmvToUpdateList();
	}

	ShaderLibrary* matsys_material_get_shaderlibrary(Material* pInternal)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();
		return reinterpret_cast<ShaderLibrary*>(mat.shaderLibrary);
	}

	Result Material_internal::create(ShaderLibrary_internal* pShaderLibrary, bool dynamic, bool addToUpdateList_)
	{
		this->dynamic = dynamic;
		this->shaderLibrary = pShaderLibrary;

		MaterialInfo& info = shaderLibrary->matInfo;

		if (info.bufferSizesCount) {

			// Reserve rawdata and initialize to 0
			rawMemory = new ui8[info.bufferSizesCount];
			svZeroMemory(rawMemory, info.bufferSizesCount);

			// Asign memory ptrs
			size_t offset = 0u;

			for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {

				MaterialBuffer& buf = buffers[i];
				size_t size = size_t(info.bufferSizes[i]);

				if (size) {
					buf.rawData = rawMemory + offset;
					offset += size;
				}
			}
			SV_ASSERT(offset == info.bufferSizesCount);
		}

		// Reserve and initialize to 0 texture ptrs
		textures.resize(info.textures.size());

		// Add to update list
		if (addToUpdateList_) addToUpdateList();

		return Result_Success;
	}

	Result Material_internal::destroy()
	{
		// Deallocate rawdata and destroy buffers
		{
			if (rawMemory) {
				delete[] rawMemory;
				rawMemory = nullptr;

				MaterialBuffer* it = buffers;
				MaterialBuffer* end = it + ShaderType_GraphicsCount;

				while (it != end) {
					if (it->rawData) {
						svCheck(graphics_destroy(it->buffer));
					}
					++it;
				}
			}
		}

		// Remove from update list
		rmvToUpdateList();

		return Result_Success;
	}

	Result Material_internal::recreate(ShaderLibrary_internal& lib)
	{
		if (&lib == shaderLibrary) return Result_Success;

		Material_internal aux;
		aux.create(&lib, dynamic, false);

		ShaderLibrary_internal& old = *shaderLibrary;

		const MaterialInfo& oldInfo = old.matInfo;
		const MaterialInfo& newInfo = lib.matInfo;

		ui32 oldSubShaderCount = old.type->subShaderCount;

		// Attributes
		for (ui32 i = 0u; i < oldInfo.attributes.size(); ++i) {

			const MaterialAttribute& oldAtt = oldInfo.attributes[i];

			// Find if it exist
			for (ui32 j = 0u; j < newInfo.attributes.size(); ++j) {
				const MaterialAttribute& newAtt = newInfo.attributes[j];

				// If exist
				if (oldAtt.type == newAtt.type && strcmp(oldAtt.name.c_str(), newAtt.name.c_str()) == 0) {

					const MaterialAttribute& att = oldAtt;

					ui32 typeSize = graphics_shader_attribute_size(att.type);
					XMMATRIX rawData;

					// Get the raw value
					const SubShaderIndices& srcIndices = oldInfo.attributeOffsets[i];

					for (ui32 w = 0; w < oldSubShaderCount; ++w) {

						ui32 index = srcIndices.i[w];

						if (index != ui32_max) {
							memcpy(&rawData, &buffers[w].rawData[index], typeSize);
							break;
						}
					}

					// Set the raw value
					const SubShaderIndices& dstIndices = newInfo.attributeOffsets[j];

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

		// Textures
		for (ui32 i = 0u; i < oldInfo.textures.size(); ++i) {

			const std::string& oldTexName = oldInfo.textures[i];

			for (ui32 j = 0u; j < newInfo.textures.size(); ++j) {

				const std::string& newTexName = newInfo.textures[i];

				if (strcmp(oldTexName.c_str(), newTexName.c_str()) == 0) {

					MaterialTexture& tex = textures[i];
					aux.textures[j] = std::move(tex);
					break;
				}
			}
		}

		destroy();
		*this = std::move(aux);
		addToUpdateList();

		return Result_Success;
	}

	void Material_internal::update(CommandList cmd)
	{
		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {

			MaterialBuffer& buffer = buffers[i];
			if (buffer.rawData == nullptr) continue;

			// Create buffer
			if (buffer.buffer == nullptr) {
				GPUBufferDesc desc;
				desc.bufferType = GPUBufferType_Constant;
				desc.CPUAccess = CPUAccess_Write;
				desc.usage = dynamic ? ResourceUsage_Dynamic : ResourceUsage_Default;
				desc.pData = nullptr;
				desc.size = shaderLibrary->matInfo.bufferSizes[i];

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

	void Material_internal::addToUpdateList()
	{
		if (!inUpdateList) {
			g_MaterialsToUpdateMutex.lock();
			g_MaterialsToUpdate.push_back(this);
			g_MaterialsToUpdateMutex.unlock();
			inUpdateList = true;
		}
	}

	void Material_internal::rmvToUpdateList()
	{
		if (inUpdateList) {
			inUpdateList = false;
			std::lock_guard<std::mutex> lock(g_MaterialsToUpdateMutex);

			for (auto it = g_MaterialsToUpdate.begin(); it != g_MaterialsToUpdate.end(); ++it) {
				if (*(it) == this) {
					g_MaterialsToUpdate.erase(it);
					break;
				}
			}
		}
	}

	void Material_internal::update()
	{
		g_MaterialsToUpdateMutex.lock();
		if (!g_MaterialsToUpdate.empty()) {
			CommandList cmd = graphics_commandlist_get();
			for (Material_internal* mat : g_MaterialsToUpdate) {
				mat->update(cmd);
				mat->inUpdateList = false;
			}
			g_MaterialsToUpdate.clear();
		}
		g_MaterialsToUpdateMutex.unlock();
	}

	Result matsys_material_attribute_set(Material* pInternal, ShaderAttributeType type, const char* name, const void* data)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		//TODO: Should update a global raw data and at update time send the data to the GPU in all the diferents buffers

		const MaterialInfo& info = mat.shaderLibrary->matInfo;
		ui32 subShaderCount = mat.shaderLibrary->type->subShaderCount;

		for (size_t i = 0u; i < info.attributes.size(); ++i) {

			const MaterialAttribute& att = info.attributes[i];

			if (att.type == type && strcmp(att.name.c_str(), name) == 0) {

				for (ui32 j = 0; j < subShaderCount; ++j) {

					ui32 index = info.attributeOffsets[i].i[j];
					if (index != ui32_max) {

						ui32 typeSize = graphics_shader_attribute_size(type);

						MaterialBuffer& mBuf = mat.buffers[j];
						memcpy(mBuf.rawData + index, data, typeSize);

						// Set begin and end indices
						mBuf.updateBegin = std::min(mBuf.updateBegin, index);
						mBuf.updateEnd = std::max(mBuf.updateEnd, index + typeSize);
					}
				}

				mat.addToUpdateList();
				return Result_Success;
			}
		}

		return Result_NotFound;
	}

	Result matsys_material_attribute_get(Material* pInternal, ShaderAttributeType type, const char* name, void* data)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		const MaterialInfo& info = mat.shaderLibrary->matInfo;
		ui32 subShaderCount = mat.shaderLibrary->type->subShaderCount;

		for (size_t i = 0u; i < info.attributes.size(); ++i) {

			const MaterialAttribute& att = info.attributes[i];

			if (att.type == type && strcmp(att.name.c_str(), name) == 0) {

				for (ui32 j = 0; j < subShaderCount; ++j) {

					ui32 index = info.attributeOffsets[i].i[j];
					if (index != ui32_max) {
						MaterialBuffer& mBuf = mat.buffers[j];
						memcpy(data, mBuf.rawData + index, graphics_shader_attribute_size(type));
						return Result_Success;
					}
				}

				mat.addToUpdateList();
				return Result_Success;
			}
		}

		return Result_NotFound;
	}

	Result matsys_material_texture_set(Material* pInternal, const char* name, GPUImage* image)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		const MaterialInfo& info = mat.shaderLibrary->matInfo;

		for (ui32 i = 0u; i < info.textures.size(); ++i) {

			const std::string& texName = info.textures[i];

			if (strcmp(texName.c_str(), name) == 0) {
				MaterialTexture& tex = mat.textures[i];
				tex.texture.unload();
				tex.image = image;
				return Result_Success;
			}
		}

		return Result_NotFound;
	}

	Result matsys_material_texture_set(Material* pInternal, const char* name, TextureAsset& texture)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		const MaterialInfo& info = mat.shaderLibrary->matInfo;

		for (ui32 i = 0u; i < info.textures.size(); ++i) {

			const std::string& texName = info.textures[i];

			if (strcmp(texName.c_str(), name) == 0) {
				MaterialTexture& tex = mat.textures[i];
				tex.texture = texture;
				tex.image = nullptr;
				return Result_Success;
			}
		}

		return Result_NotFound;
	}

	Result matsys_material_texture_get(Material* pInternal, const char* name, GPUImage** pImage)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		const MaterialInfo& info = mat.shaderLibrary->matInfo;

		for (ui32 i = 0u; i < info.textures.size(); ++i) {

			const std::string& texName = info.textures[i];

			if (strcmp(texName.c_str(), name) == 0) {
				MaterialTexture& tex = mat.textures[i];
				if (tex.texture.hasReference())
					*pImage = tex.texture.get();
				else
					*pImage = tex.image;
				return Result_Success;
			}
		}

		return Result_NotFound;
	}

	Result matsys_material_texture_get(Material* pInternal, const char* name, TextureAsset& texture)
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		const MaterialInfo& info = mat.shaderLibrary->matInfo;

		for (ui32 i = 0u; i < info.textures.size(); ++i) {

			const std::string& texName = info.textures[i];

			if (strcmp(texName.c_str(), name) == 0) {
				texture = mat.textures[i].texture;
				return Result_Success;
			}
		}

		return Result_NotFound;
	}

}