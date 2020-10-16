#pragma once

#include "rendering/material_system.h"

#define svThrow(x) SV_THROW("MATERIAL_SYSTEM_ERROR", x)
#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[MATERIAL_SYSTEM] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[MATERIAL_SYSTEM_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[MATERIAL_SYSTEM_ERROR] "#x, __VA_ARGS__)

#define svLogCompile(x, ...) sv::console_log(sv::LoggingStyle_Red, "[SHADER COMPILE] "#x, __VA_ARGS__)

namespace sv {

	struct ShaderIndices {
		ui32 i[ShaderType_GraphicsCount];

		ShaderIndices()
		{
			memset(i, ui32_max, sizeof(ui32) * ShaderType_GraphicsCount);
		}
	};

	struct Material_internal;

	struct ShaderLibrary_internal {

		Shader* vs = nullptr;
		Shader* ps = nullptr;
		Shader* gs = nullptr;

		std::vector<ShaderLibraryAttribute>	attributes; // Sorted list -> Textures - buffer data. Aligned with attributeIndices
		std::vector<ShaderIndices>			attributeIndices; // Contains information about attr. offset (or texture slot) and if a shader contains or not this attr.
		ui32								bufferSizes[ShaderType_GraphicsCount] = {};
		ui32								bufferBindigs[ShaderType_GraphicsCount] = {};
		ui32								bufferSizesCount = 0u;
		ui32								texturesCount = 0u;
		std::vector<Material_internal*>		materials;
		ShaderIndices						cameraBufferBinding;
		
		std::string							name;
		size_t								hashCode;

		inline Shader* get_shader(ui32 type) const noexcept 
		{
			switch (type)
			{
			case sv::ShaderType_Vertex:
				return vs;
			case sv::ShaderType_Pixel:
				return ps;
			case sv::ShaderType_Geometry:
				return gs;
			default:
				return nullptr;
			}
		}

	};

	struct MaterialBuffer {
		GPUBuffer* buffer = nullptr;
		ui8* rawData = nullptr;

		ui32 updateBegin = 0u;
		ui32 updateEnd = 0u;
	};

	struct Material_internal {

		ShaderLibrary_internal* shaderLibrary;
		MaterialBuffer			buffers[ShaderType_GraphicsCount] = {};
		std::vector<GPUImage*>	textures;
		bool					dynamic;
		bool					inUpdateList = false;
		ui8*					rawMemory = nullptr;

	};

	struct CameraBuffer_internal {

		GPUBuffer* buffer;
		
		struct BufferData {
			XMMATRIX viewMatrix;
			XMMATRIX projectionMatrix;
			XMMATRIX viewProjectionMatrix;
			vec3f position;
			float padding;
			vec3f direction;
		} data;

	};

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

	Result	matsys_initialize();
	void	matsys_update();
	Result	matsys_close();

	Result matsys_shaderlibrary_compile(ShaderLibrary_internal& lib, const char* src);
	Result matsys_shaderlibrary_construct(ShaderLibrary_internal& lib);

	void matsys_materials_close();
	void matsys_materials_update();
	void matsys_material_updatelist_add(Material_internal* mat);
	void matsys_material_updatelist_rmv(Material_internal* mat);
	void matsys_material_update(Material_internal& mat, CommandList cmd);

}


/* SHADER RECREATE CODE
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
*/

/* MATERIAL RECREATE CODE
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

						const ShaderLibraryAttribute& oldAttr = oldLib.attributes[j];
						ui32 oldIndex = oldLib.attributeIndices[j].i[i];
						if (oldIndex == ui32_max) continue;

						// Find this attribute index
						ui32 newIndex = ui32_max;
						for (ui32 w = 0; w < newLib.attributes.size(); ++w) {
							const ShaderLibraryAttribute& a = newLib.attributes[w];
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
	*/