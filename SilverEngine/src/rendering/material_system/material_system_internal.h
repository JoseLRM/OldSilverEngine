#pragma once

#include "rendering/material_system.h"

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
		std::string							type;
		size_t								nameHashCode;

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

	struct MaterialTexture {
		GPUImage* image = nullptr;
		TextureAsset texture;
	};

	struct Material_internal {

		ShaderLibrary_internal*			shaderLibrary;
		MaterialBuffer					buffers[ShaderType_GraphicsCount] = {};
		std::vector<MaterialTexture>	textures;
		bool							dynamic;
		bool							inUpdateList = false;
		ui8*							rawMemory = nullptr;

	};

	struct CameraBuffer_internal {

		GPUBuffer* buffer;
		
		struct BufferData {
			XMMATRIX viewMatrix;
			XMMATRIX projectionMatrix;
			XMMATRIX viewProjectionMatrix;
			vec3f position;
			float padding;
			vec4f direction;
		} data;

	};

	struct MaterialAsset_internal {

		Material material;
		size_t hashCode;

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

	void	matsys_materials_close();
	void	matsys_materials_update();
	void	matsys_material_updatelist_add(Material_internal* mat);
	void	matsys_material_updatelist_rmv(Material_internal* mat);
	Result	matsys_material_create(Material_internal& mat, ShaderLibrary_internal* shaderLibrary, bool dynamic, bool addToUpdateList);
	Result	matsys_material_destroy(Material_internal& mat);
	Result	matsys_material_recreate(Material_internal& mat, ShaderLibrary_internal& lib);
	void	matsys_material_update(Material_internal& mat, CommandList cmd);

	Result matsys_asset_register();

}