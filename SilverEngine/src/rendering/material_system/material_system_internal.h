#pragma once

#include "rendering/material_system.h"
#include "utils/allocator.h"

namespace sv {

	constexpr ui32 MAX_SUBSHADERS = 10u;

	struct SubShaderRegister {

		std::string name;
		std::string preLibName;
		std::string postLibName;
		Shader*		defaultShader;
		ShaderType	type;

	};

	struct ShaderLibraryType_internal {

		std::string			name;
		SubShaderRegister	subShaderRegisters[MAX_SUBSHADERS];
		ui32				subShaderCount;

		SubShaderID findSubShaderID(const char* name);

	};

	struct SubShader {

		Shader* shader = nullptr;
		ui32 cameraSlot = ui32_max;

	};

	struct SubShaderIndices {
		ui32 i[MAX_SUBSHADERS]; // Buffer offsets, aligned with subshader list

		inline void init(ui32 n) { for (ui32 j = 0u; j < MAX_SUBSHADERS; ++j) i[j] = n; }
	};

	struct MaterialInfo {

		std::vector<MaterialAttribute>	attributes; // Buffer data. Aligned with attributeOffsets
		std::vector<SubShaderIndices>	attributeOffsets;
		
		ui32 bufferSizes[MAX_SUBSHADERS];		// Per subshader: buffer size
		ui32 bufferBindings[MAX_SUBSHADERS];	// Per subshader: buffer binding

		ui32 bufferSizesCount = 0u; // The sum of all the material attributes

		std::vector<std::string>		textures;			// texture names. Aligned with textureSlots
		std::vector<SubShaderIndices>	textureSlots;		// textures slots
		
	};

	struct ShaderLibrary_internal {

		SubShader subShaders[MAX_SUBSHADERS];
		GPUBuffer* cameraBuffer = nullptr;

		MaterialInfo matInfo;
		
		std::atomic<i32>	matCount = 0u;
		std::string			name;
		ShaderLibraryType_internal*	type;
		size_t				nameHashCode;

		ShaderLibrary_internal() = default;
		ShaderLibrary_internal& operator=(const ShaderLibrary_internal& other);
		ShaderLibrary_internal& operator=(ShaderLibrary_internal&& other) noexcept;

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
		MaterialBuffer					buffers[MAX_SUBSHADERS];
		std::vector<MaterialTexture>	textures;
		bool							dynamic;
		bool							inUpdateList = false;
		ui8*							rawMemory = nullptr;

		Result create(ShaderLibrary_internal* pShaderLibrary, bool dynamic, bool addToUpdateList);
		Result destroy();
		Result recreate(ShaderLibrary_internal& lib);
		void update(CommandList cmd);
		void addToUpdateList();
		void rmvToUpdateList();

		static void update();

	};

	struct MaterialAsset_internal {

		Material* material = nullptr;
		size_t hashCode = 0u;

	};

	extern InstanceAllocator<ShaderLibraryType_internal, 10u>		g_ShaderLibrariesTypes;
	extern InstanceAllocator<ShaderLibrary_internal, 50u>			g_ShaderLibraries;
	extern InstanceAllocator<Material_internal, 100u>				g_Materials;

	extern std::unordered_map<std::string, ShaderLibraryType_internal*> g_ShaderLibraryTypeNames;

	extern std::vector<Material_internal*>					g_MaterialsToUpdate;
	extern std::mutex										g_MaterialsToUpdateMutex;

	static inline ShaderLibraryType_internal* typeFind(const char* name)
	{
		auto it = g_ShaderLibraryTypeNames.find(name);
		if (it == g_ShaderLibraryTypeNames.end()) return nullptr;
		else return it->second;
	}

#define ASSERT_PTR() SV_ASSERT(pInternal != nullptr)

#define PARSE_SHADER_LIBRARY() ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(pInternal)
#define PARSE_MATERIAL() Material_internal& mat = *reinterpret_cast<Material_internal*>(pInternal)
#define PARSE_CAMERA_BUFFER() CameraBuffer_internal& cam = *reinterpret_cast<CameraBuffer_internal*>(pInternal)

	Result	matsys_initialize();
	void	matsys_update();
	Result	matsys_close();

	Result matsys_shaderlibrary_compile(ShaderLibrary_internal& lib, const char* src);

	Result matsys_asset_register();

}