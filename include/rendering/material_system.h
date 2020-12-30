#pragma once

#include "platform/graphics.h"
#include "simulation/asset_system.h"

#define SV_MATSYS_MAX_SUBSHADERS 10u

namespace sv {

	struct MaterialAttribute {
		std::string			name;
		ShaderAttributeType type;
	};

	struct CameraBuffer;

	SV_DEFINE_HANDLE(ShaderLibrary);
	SV_DEFINE_HANDLE(Material);

	typedef u32 SubShaderID;

	// Shader Library

	Result matsys_shaderlibrary_create_from_file(const char* filePath, ShaderLibrary** pShaderLibrary);
	Result matsys_shaderlibrary_create_from_string(const char* src, ShaderLibrary** pShaderLibrary);
	Result matsys_shaderlibrary_create_from_binary(const char* name, ShaderLibrary** pShaderLibrary);
	Result matsys_shaderlibrary_create_from_binary(size_t hashCode, ShaderLibrary** pShaderLibrary);

	Result matsys_shaderlibrary_destroy(ShaderLibrary* shaderLibrary);

	void matsys_shaderlibrary_bind_subshader(ShaderLibrary* shaderLibrary, SubShaderID subShaderID, CommandList cmd);
	void matsys_shaderlibrary_bind_camerabuffer(ShaderLibrary* shaderLibrary, CameraBuffer& cameraBuffer, CommandList cmd);

	const std::vector<MaterialAttribute>&	matsys_shaderlibrary_attributes(ShaderLibrary* shaderLibrary);
	const std::vector<std::string>&			matsys_shaderlibrary_textures(ShaderLibrary* shaderLibrary);

	// Material

	Result matsys_material_create(ShaderLibrary* shaderLibrary, bool dynamic, Material** pMaterial);
	Result matsys_material_destroy(Material* material);

	void matsys_material_bind(Material* material, SubShaderID subShaderID, CommandList cmd);
	void matsys_material_update(Material* material, CommandList cmd);

	ShaderLibrary* matsys_material_get_shaderlibrary(Material* material);

	// Material Attributes

	Result matsys_material_attribute_set(Material* material, ShaderAttributeType type, const char* name, const void* data);
	Result matsys_material_attribute_get(Material* material, ShaderAttributeType type, const char* name, void* data);

	Result matsys_material_texture_set(Material* material, const char* name, GPUImage* image);
	Result matsys_material_texture_set(Material* material, const char* name, TextureAsset& texture);
	Result matsys_material_texture_get(Material* material, const char* name, GPUImage** pImage);
	Result matsys_material_texture_get(Material* material, const char* name, TextureAsset& texture);

	// ShaderLibrary Types

	struct ShaderLibraryTypeDesc {
		
		const char*	name;
		const char* subshaderIncludeNames[SV_MATSYS_MAX_SUBSHADERS];
		u32		subShaderCount;

	};

	Result				matsys_shaderlibrary_type_register(const ShaderLibraryTypeDesc* desc);
	SubShaderID			matsys_subshader_get(const char* typeName, const char* subShaderName); // Return u32_max if not found

	// Camera Buffer

	struct CameraBuffer {

		~CameraBuffer();

		GPUBuffer* gpuBuffer = nullptr;
		XMMATRIX	viewMatrix = XMMatrixIdentity();
		XMMATRIX	projectionMatrix = XMMatrixIdentity();
		vec3f		position;
		vec4f		direction;

		Result updateGPU(CommandList cmd);
		Result clear();

	};

	// Assets

	class ShaderLibraryAsset : public Asset {
	public:
		inline ShaderLibrary* get() const noexcept { return reinterpret_cast<ShaderLibrary*>(m_Ref.get()); }
		inline ShaderLibrary* operator->() const noexcept { return get(); }
	};

	class MaterialAsset : public Asset {
	public:
		inline Material* get() const noexcept { void* inter = m_Ref.get(); return inter ? *reinterpret_cast<Material**>(inter) : nullptr; }
		inline Material* operator->() const noexcept { return get(); }

		Result createFile(const char* filePath, ShaderLibraryAsset& shaderLibraryAsset);
	};

}