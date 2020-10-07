#include "core.h"

#include "material_system_internal.h"

#include "utils/allocators/InstanceAllocator.h"

namespace sv {

	static InstanceAllocator<ShaderLibrary_internal>		g_ShaderLibraries;
	static InstanceAllocator<Material_internal>				g_Materials;

	static std::vector<Material_internal*>	g_MaterialsToUpdate;
	static std::mutex						g_MaterialsToUpdateMutex;

	inline void matsys_material_add_update_list(Material_internal* mat) 
	{
		if (!mat->inUpdateList) {
			g_MaterialsToUpdateMutex.lock();
			g_MaterialsToUpdate.push_back(mat);
			g_MaterialsToUpdateMutex.unlock();
			mat->inUpdateList = true;
		}
	}

	Result matsys_initialize()
	{
		return Result_Success;
	}

	void matsys_update()
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

	Result matsys_close()
	{
		ui32 count;

		count = g_ShaderLibraries.unfreed_count();
		if (count) svLogWarning("There are %u unfreed Shader Libraries", count);

		count = g_Materials.unfreed_count();
		if (count) svLogWarning("There are %u unfreed Materials", count);

		g_ShaderLibraries.clear();
		g_Materials.clear();
		g_MaterialsToUpdate.clear();

		return Result_Success;
	}

	constexpr bool string_equal(const char* s0, size_t size0, const char* s1, size_t size1) 
	{
		if (size0 != size1) return false;

		const char* end0 = s0 + size0;
		const char* end1 = s1 + size1;
		
		while (s0 != end0) {

			if (*s0 != *s1) return false;

			++s0;
			++s1;
		}

		return true;
	}

	constexpr bool attribute_equal(const ShaderAttribute& a0, const ShaderAttribute& a1) 
	{
		return a0.type == a1.type && string_equal(a0.name.c_str(), a0.name.size(), a1.name.c_str(), a1.name.size());
	}

	/////////////////////////////// SHADER LIBRARY CREATION AND DESTRUCTION /////////////////////////////// 

	void matsys_shaderlibrary_textures_add(const ShaderMaterialInfo* matInfo, ShaderType shaderType, std::vector<ShaderAttribute>& attributes, std::vector<ShaderIndices>& indices)
	{
		ui32 beginSize = attributes.size();
		ui32 slotCount = 0u;

		const ShaderAttribute* src = matInfo->attributes.data();
		const ShaderAttribute* endSrc = matInfo->attributes.data() + matInfo->texturesSlots.size();

		while (src != endSrc) {

			const ShaderAttribute& attr = *src;

			// Find if exist
			ui32 i;
			for (i = 0; i < beginSize; ++i) {
				if (attribute_equal(attributes[i], attr)) {
					break;
				}
			}

			// If not exist add new
			if (i == beginSize) {
				i = ui32(attributes.size());
				attributes.emplace_back() = attr;
				indices.emplace_back();
			}

			// Set texture slot
			indices[i].i[shaderType] = matInfo->texturesSlots[slotCount++];

			++src;
		}

	}

	void matsys_shaderlibrary_attributes_add(const ShaderMaterialInfo* matInfo, ShaderType shaderType, std::vector<ShaderAttribute>& attributes, std::vector<ShaderIndices>& indices, size_t textureCount)
	{
		ui32 beginSize = attributes.size();
		ui32 offset = 0u;
		ui32 slotCount = 0u;

		const ShaderAttribute* src = matInfo->attributes.data() + textureCount;
		const ShaderAttribute* endSrc = matInfo->attributes.data() + matInfo->attributes.size();

		while (src != endSrc) {

			const ShaderAttribute& attr = *src;

			// Find if exist
			ui32 i;
			for (i = 0; i < beginSize; ++i) {
				if (attribute_equal(attributes[i], attr)) {
					break;
				}
			}

			// If not exist add new
			if (i == beginSize) {
				i = ui32(attributes.size());
				attributes.emplace_back() = attr;
				indices.emplace_back();
			}

			// Compute offset if it's a buffer data
			ui32 typeSize = graphics_shader_attribute_size(attr.type);
			SV_ASSERT(typeSize != 0u);

			if (offset / 16u != (offset + std::min(typeSize, 15u)) / 16) offset += 16 - (offset % 16);

			indices[i].i[shaderType] = offset;

			offset += typeSize;

			++src;
		}
		
	}

	Result matsys_shaderlibrary_construct(const ShaderLibraryDesc* desc, ShaderLibrary_internal& lib)
	{
		if (desc->vertexShader == nullptr) {
			svLogError("Shader Library must have valid vertex shader");
			return Result_InvalidUsage;
		}

		lib.vs = desc->vertexShader;
		lib.ps = desc->pixelShader;
		lib.gs = desc->geometryShader;

		const ShaderMaterialInfo* matInfo;

		// Textures
		{
			// Add VertexShader textures
			matInfo = graphics_shader_materialinfo_get(lib.vs);
			matsys_shaderlibrary_textures_add(matInfo, ShaderType_Vertex, lib.attributes, lib.attributeIndices);

			// Add PixelShader textures
			if (lib.ps) {
				matInfo = graphics_shader_materialinfo_get(lib.ps);
				matsys_shaderlibrary_textures_add(matInfo, ShaderType_Pixel, lib.attributes, lib.attributeIndices);
			}

			// Add GeometryShader textures
			if (lib.gs) {
				matInfo = graphics_shader_materialinfo_get(lib.gs);
				matsys_shaderlibrary_textures_add(matInfo, ShaderType_Geometry, lib.attributes, lib.attributeIndices);
			}
		}

		lib.texturesCount = ui32(lib.attributes.size());

		// Buffer data
		{
			// Add VertexShader buffer data
			matInfo = graphics_shader_materialinfo_get(lib.vs);
			matsys_shaderlibrary_attributes_add(matInfo, ShaderType_Vertex, lib.attributes, lib.attributeIndices, matInfo->texturesSlots.size());

			// Add PixelShader buffer data
			if (lib.ps) {
				matInfo = graphics_shader_materialinfo_get(lib.ps);
				matsys_shaderlibrary_attributes_add(matInfo, ShaderType_Pixel, lib.attributes, lib.attributeIndices, matInfo->texturesSlots.size());
			}

			// Add GeometryShader buffer data
			if (lib.gs) {
				matInfo = graphics_shader_materialinfo_get(lib.gs);
				matsys_shaderlibrary_attributes_add(matInfo, ShaderType_Geometry, lib.attributes, lib.attributeIndices, matInfo->texturesSlots.size());
			}
		}

		// Compute attributes buffer size per shader
		auto& indices = lib.attributeIndices;

		if (indices.size() > lib.texturesCount) {
			for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {

				ui32 lastOffsetIndex = ui32_max;

				for (ui32 j = lib.texturesCount; j < lib.attributeIndices.size(); ++j) {
					ui32 offset = indices[j].i[i];
					if (offset != ui32_max) {
						if (lastOffsetIndex == ui32_max || offset > indices[lastOffsetIndex].i[i])
							lastOffsetIndex = j;
					}
				}

				if (lastOffsetIndex == ui32_max) continue;

				const ShaderIndices& index = lib.attributeIndices[lastOffsetIndex];
				const ShaderAttribute& type = lib.attributes[lastOffsetIndex];

				// Last attribute offset + his size
				lib.bufferSizes[i] = index.i[i] + graphics_shader_attribute_size(type.type);
				// TODO: Should align the size ??
			}
		}

		return Result_Success;
	}

	Result matsys_shaderlibrary_create(const ShaderLibraryDesc* desc, ShaderLibrary** pShaderLibrary)
	{
		ShaderLibrary_internal lib;
		Result res = matsys_shaderlibrary_construct(desc, lib);

		if (res == Result_Success) {
			ShaderLibrary_internal* s = g_ShaderLibraries.create();
			*s = std::move(lib);
			*pShaderLibrary = (ShaderLibrary*)s;
		}
		
		return res;
	}

	Result matsys_shaderlibrary_recreate(const ShaderLibraryDesc* desc, ShaderLibrary* shaderLibrary_)
	{
		SV_ASSERT(shaderLibrary_);

		ShaderLibrary_internal aux;
		svCheck(matsys_shaderlibrary_construct(desc, aux));

		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);

		// Recrate materials
		for (Material_internal* mat : lib.materials) {
			svCheck(matsys_material_recreate(reinterpret_cast<ShaderLibrary*>(&aux), reinterpret_cast<Material*>(mat)));
			mat->shaderLibrary = &lib;
		}

		lib = std::move(aux);

		return Result_Success;
	}

	Result matsys_shaderlibrary_destroy(ShaderLibrary* shaderLibrary)
	{
		SV_ASSERT(shaderLibrary);
		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary);

		if (!lib.materials.empty()) {
			svLogError("Can't destroy a Shader Library with Materials");
			return Result_InvalidUsage;
		}

		// Deallocate
		g_ShaderLibraries.destroy(&lib);
		return Result_Success;
	}

	/////////////////////////////// SHADER LIBRARY UTILS /////////////////////////////// 

	const std::vector<ShaderAttribute>& matsys_shaderlibrary_attributes_get(ShaderLibrary* shaderLibrary_)
	{
		SV_ASSERT(shaderLibrary_);
		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);
		return lib.attributes;
	}

	ui32 matsys_shaderlibrary_texture_count(ShaderLibrary* shaderLibrary_)
	{
		SV_ASSERT(shaderLibrary_);
		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);
		return lib.texturesCount;
	}

	Shader* matsys_shaderlibrary_shader_get(ShaderLibrary* shaderLibrary_, ShaderType shaderType)
	{
		SV_ASSERT(shaderLibrary_);
		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);

		switch (shaderType)
		{
		case ShaderType_Vertex:
			return lib.vs;
		case ShaderType_Pixel:
			return lib.ps;
		case ShaderType_Geometry:
			return lib.gs;
		default:
			return nullptr;
		}
	}

	void matsys_shaderlibrary_bind(ShaderLibrary* shaderLibrary_, CommandList cmd)
	{
		SV_ASSERT(shaderLibrary_);
		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);

		// Bind shaders
		SV_ASSERT(lib.vs != nullptr);
		graphics_shader_bind(lib.vs, cmd);
		if (lib.ps) graphics_shader_bind(lib.ps, cmd);
		if (lib.gs) graphics_shader_bind(lib.gs, cmd);
	}

	/////////////////////////////// MATERIAL CREATION AND DESTRUCTION /////////////////////////////// 

	Result matsys_material_create(const MaterialDesc* desc, Material** pMaterial)
	{
		if (desc->shaderLibrary == nullptr) return Result_InvalidUsage;

		Material_internal mat;
		mat.shaderLibrary = reinterpret_cast<ShaderLibrary_internal*>(desc->shaderLibrary);
		mat.dynamic = desc->dynamic;

		if (!mat.shaderLibrary->attributeIndices.empty()) {

			// Reserve rawdata and initialize to 0
			{
				for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
					MaterialBuffer& buf = mat.buffers[i];
					size_t size = size_t(mat.shaderLibrary->bufferSizes[i]);
					if (size) {
						buf.rawData = new ui8[size];
						svZeroMemory(buf.rawData, size);
					}
				}
			}
		}
		
		// Reserve and initialize to 0 texture ptrs
		mat.textures.resize(mat.shaderLibrary->texturesCount, nullptr);
		
		// Allocate material and move created material
		{
			Material_internal* result = g_Materials.create();
			*result = std::move(mat);
			result->shaderLibrary->materials.push_back(result);
			matsys_material_add_update_list(result);
			*pMaterial = (Material*)result;
			return Result_Success;
		}
	}

	void matsys_material_remove_shader_reference(Material_internal* mat, ShaderLibrary_internal* lib)
	{
		auto& materials = lib->materials;
		for (auto it = materials.begin(); it != materials.end(); ++it) {
			if (*it._Ptr == mat) {
				materials.erase(it);
				break;
			}
		}
	}

	Result matsys_material_recreate(ShaderLibrary* shaderLibrary_, Material* material_)
	{
		if (shaderLibrary_ == nullptr || material_ == nullptr) return Result_InvalidUsage;
		Material_internal& mat = *reinterpret_cast<Material_internal*>(material_);

		ShaderLibrary_internal& oldLib = *mat.shaderLibrary;
		ShaderLibrary_internal& newLib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);

		bool mustUpdate = false;

		// Update attributes
		{
			// Resize rawdata and destroy invalid buffers
			for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {

				MaterialBuffer& buffer = mat.buffers[i];

				ui32 newSize = newLib.bufferSizes[i];
				ui32 oldSize = oldLib.bufferSizes[i];

				// No attributes
				if (newSize == 0u) {
					if (buffer.rawData) {
						delete[] buffer.rawData;
					}
					svCheck(graphics_destroy(buffer.buffer));
					buffer.buffer = nullptr;
					continue;
				}

				// Check if must update rawdata
				if (newSize == oldSize && oldLib.attributes.size() == newLib.attributes.size()) {
					ui32 j;
					for (j = 0; j < oldLib.attributes.size(); ++j) {
						const auto& a0 = oldLib.attributes[j];
						const auto& a1 = newLib.attributes[j];

						if (a0.type != a1.type || !string_equal(a0.name.c_str(), a0.name.size(), a1.name.c_str(), a1.name.size())) {
							break;
						}
					}
					if (j == oldLib.attributes.size()) continue;
				}

				// Set new material data
				mustUpdate = true;
				ui8* newData = new ui8[newSize];
				svZeroMemory(newData, newSize);

				if (oldSize != 0u) {
					for (ui32 j = 0; j < oldLib.attributes.size(); ++j) {

						const ShaderAttribute& oldAttr = oldLib.attributes[j];
						ui32 oldIndex = oldLib.attributeIndices[j].i[i];
						if (oldIndex == ui32_max) continue;

						// Find this attribute index
						ui32 newIndex = ui32_max;
						for (ui32 w = 0; w < newLib.attributes.size(); ++w) {
							const ShaderAttribute& a = newLib.attributes[w];
							if (oldAttr.type == a.type && string_equal(oldAttr.name.c_str(), oldAttr.name.size(), a.name.c_str(), a.name.size())) {
								newIndex = newLib.attributeIndices[w].i[i];;
								break;
							}
						}

						// Copy attribute value to new data
						if (newIndex != ui32_max) {
							memcpy(newData + newIndex, buffer.rawData + oldIndex, graphics_shader_attribute_size(oldAttr.type));
						}
					}

					if (buffer.rawData) {
						delete[] buffer.rawData;
						buffer.rawData = nullptr;
					}

					if (oldSize != newSize) {
						svCheck(graphics_destroy(buffer.buffer));
						buffer.buffer = nullptr;
					}
					buffer.updateBegin = 0u;
					buffer.updateEnd = newSize;
				}

				buffer.rawData = newData;
			}
		}

		// Update Textures
		//TODO: 
		mat.textures.resize(newLib.texturesCount, nullptr);
		
		if (mustUpdate) matsys_material_add_update_list(&mat);
		matsys_material_remove_shader_reference(&mat, mat.shaderLibrary);
		mat.shaderLibrary = reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);
		mat.shaderLibrary->materials.push_back(&mat);
		return Result_Success;
	}

	Result matsys_material_destroy(Material* material_)
	{
		if (material_ == nullptr) return Result_Success;
		Material_internal& mat = *reinterpret_cast<Material_internal*>(material_);

		// Deallocate rawdata and destroy buffers
		{
			MaterialBuffer* it = mat.buffers;
			MaterialBuffer* end = it + ShaderType_GraphicsCount;

			while (it != end) {

				if (it->rawData) {

					svCheck(graphics_destroy(it->buffer));
					delete[] it->rawData;

				}

				++it;
			}
		}

		// Destroy shaderlibrary reference
		matsys_material_remove_shader_reference(&mat, mat.shaderLibrary);

		// Remove from update list
		if (mat.inUpdateList) {
			std::lock_guard<std::mutex> lock(g_MaterialsToUpdateMutex);

			for (auto it = g_MaterialsToUpdate.begin(); it != g_MaterialsToUpdate.end(); ++it) {
				if (*(it) == &mat) {
					g_MaterialsToUpdate.erase(it);
					break;
				}
			}
		}

		// Deallocate
		g_Materials.destroy(&mat);
		return Result_Success;
	}

	/////////////////////////////// MATERIAL UTILS /////////////////////////////// 

	Result matsys_material_set(Material* material_, const char* name, ShaderAttributeType type, const void* data)
	{
		SV_ASSERT(material_);
		Material_internal& mat = *reinterpret_cast<Material_internal*>(material_);

		size_t size = strlen(name);
		ui32 typeSize = graphics_shader_attribute_size(type);

		const ShaderAttribute* begin;
		const ShaderAttribute* it;
		const ShaderAttribute* endIt;

		if (type == ShaderAttributeType_Unknown) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {

					if (it->type == ShaderAttributeType_Texture) {
						mat.textures[it - begin] = (GPUImage*)data;
					}
					else {
						for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

							ui32 index = mat.shaderLibrary->attributeIndices[it - begin].i[j];
							if (index != ui32_max) {
								MaterialBuffer& mBuf = mat.buffers[j];
								memcpy(mBuf.rawData + index, data, typeSize);

								// Set begin and end indices
								mBuf.updateBegin = std::min(mBuf.updateBegin, index);
								mBuf.updateEnd = std::max(mBuf.updateEnd, index + typeSize);
							}
						}
						
						matsys_material_add_update_list(&mat);
					}

					return Result_Success;
				}

				++it;
			}
		}
		else if (type == ShaderAttributeType_Texture) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->texturesCount;

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
					mat.textures[it - begin] = (GPUImage*)data;
					return Result_Success;
				}

				++it;
			}
		}
		else {
			begin = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->texturesCount;
			it = begin;
			endIt = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (it->type == type && string_equal(it->name.c_str(), it->name.size(), name, size)) {

					for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

						ui32 index = mat.shaderLibrary->attributeIndices[it - begin + mat.shaderLibrary->texturesCount].i[j];
						if (index != ui32_max) {
							MaterialBuffer& mBuf = mat.buffers[j];
							memcpy(mBuf.rawData + index, data, typeSize);

							// Set begin and end indices
							mBuf.updateBegin = std::min(mBuf.updateBegin, index);
							mBuf.updateEnd = std::max(mBuf.updateEnd, index + typeSize);
						}
					}

					matsys_material_add_update_list(&mat);
					return Result_Success;
				}

				++it;
			}
		}

		return Result_NotFound;
	}

	Result matsys_material_get(Material* material_, const char* name, ShaderAttributeType type, void* data)
	{
		SV_ASSERT(material_);
		Material_internal& mat = *reinterpret_cast<Material_internal*>(material_);

		size_t size = strlen(name);
		ui32 typeSize = graphics_shader_attribute_size(type);

		const ShaderAttribute* begin;
		const ShaderAttribute* it;
		const ShaderAttribute* endIt;

		if (type == ShaderAttributeType_Unknown) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {

					if (it->type == ShaderAttributeType_Texture) {
						data = mat.textures[it - begin];
					}
					else {
						for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

							ui32 index = mat.shaderLibrary->attributeIndices[it - begin].i[j];
							if (index != ui32_max) {
								MaterialBuffer& mBuf = mat.buffers[j];
								memcpy(data, mBuf.rawData + index, typeSize);
								matsys_material_add_update_list(&mat);
								return Result_Success;
							}
						}
					}
				}

				++it;
			}
		}
		else if (type == ShaderAttributeType_Texture) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->texturesCount;

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
					data = mat.textures[it - begin];
					return Result_Success;
				}

				++it;
			}
		}
		else {
			begin = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->texturesCount;
			it = begin;
			endIt = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (it->type == type && string_equal(it->name.c_str(), it->name.size(), name, size)) {

					for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

						ui32 index = mat.shaderLibrary->attributeIndices[it - begin + mat.shaderLibrary->texturesCount].i[j];
						if (index != ui32_max) {
							MaterialBuffer& mBuf = mat.buffers[j];
							memcpy(data, mBuf.rawData + index, typeSize);
							matsys_material_add_update_list(&mat);
							return Result_Success;
						}
					}
				}

				++it;
			}
		}

		return Result_NotFound;
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

	void matsys_material_update(Material* material_, CommandList cmd)
	{
		SV_ASSERT(material_);
		Material_internal& material = *reinterpret_cast<Material_internal*>(material_);

		matsys_material_update(material, cmd);

		// Remove from update list
		if (material.inUpdateList) {
			material.inUpdateList = false;
			std::lock_guard<std::mutex> lock(g_MaterialsToUpdateMutex);
			for (auto it = g_MaterialsToUpdate.begin(); it != g_MaterialsToUpdate.end(); ++it) {
				if ((*it) == &material) {
					g_MaterialsToUpdate.erase(it);
					break;
				}
			}
		}
	}

	void matsys_material_bind(Material* material_, CommandList cmd)
	{
		SV_ASSERT(material_);
		Material_internal& mat = *reinterpret_cast<Material_internal*>(material_);

		// Textures
		for (ui32 shaderType = 0; shaderType < ShaderType_GraphicsCount; ++shaderType) {
			for (ui32 i = 0; i < mat.shaderLibrary->texturesCount; ++i) {
				ui32 bindSlot = mat.shaderLibrary->attributeIndices[i].i[shaderType];
				if (bindSlot != ui32_max)
					graphics_image_bind(mat.textures[i], bindSlot, (ShaderType)shaderType, cmd);
			}
		}

		MaterialBuffer* it = mat.buffers;
		ui32 slot;

		// Vertex Shader
		if (it->buffer) {
			slot = graphics_shader_materialinfo_get(mat.shaderLibrary->vs)->bufferSlot;
			graphics_constantbuffer_bind(it->buffer, slot, ShaderType_Vertex, cmd);
		}
		++it;

		// Pixel Shader
		if (it->buffer) {
			slot = graphics_shader_materialinfo_get(mat.shaderLibrary->ps)->bufferSlot;
			graphics_constantbuffer_bind(it->buffer, slot, ShaderType_Pixel, cmd);
		}
		++it;

		// Geometry Shader
		if (it->buffer) {
			slot = graphics_shader_materialinfo_get(mat.shaderLibrary->gs)->bufferSlot;
			graphics_constantbuffer_bind(it->buffer, slot, ShaderType_Geometry, cmd);
		}

		// TODO: more shaders... ++it;
	}

	ShaderLibrary* matsys_material_shader_get(Material* material_)
	{
		SV_ASSERT(material_);
		Material_internal& mat = *reinterpret_cast<Material_internal*>(material_);
		return (ShaderLibrary*)mat.shaderLibrary;
	}

}