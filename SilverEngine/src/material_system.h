#pragma once

#include "graphics.h"

namespace sv {

	struct ShaderLibraryDesc {
		Shader* vertexShader;
		Shader* pixelShader;
		Shader* geometryShader;
	};

	SV_DEFINE_HANDLE(ShaderLibrary);
	SV_DEFINE_HANDLE(Material);

	// Shader Library

	Result matsys_shaderlibrary_create(const ShaderLibraryDesc* desc, ShaderLibrary** pShaderLibrary);
	Result matsys_shaderlibrary_recreate(const ShaderLibraryDesc* desc, ShaderLibrary* shaderLibrary);
	Result matsys_shaderlibrary_destroy(ShaderLibrary* shaderLibrary);

	const std::vector<ShaderAttribute>& matsys_shaderlibrary_attributes_get(ShaderLibrary* shaderLibrary);
	bool								matsys_shaderlibrary_attribute_exist(ShaderLibrary* shaderLibrary, const char* name);
	ShaderAttributeType					matsys_shaderlibrary_attribute_type(ShaderLibrary* shaderLibrary, const char* name);

	Shader* matsys_shaderlibrary_shader_get(ShaderLibrary* shaderLibrary, ShaderType shaderType);

	void matsys_shaderlibrary_bind(ShaderLibrary* shaderLibrary, CommandList cmd);

	// Material

	Result matsys_material_create(ShaderLibrary* shaderLibrary, Material** pMaterial);
	Result matsys_material_recreate(ShaderLibrary* shaderLibrary, Material* material);
	Result matsys_material_destroy(Material* material);

	Result matsys_material_set(Material* material, const char* name, const void* data, ShaderAttributeType type);
	Result matsys_material_get(Material* material, const char* name, void* data, ShaderAttributeType type);

	void matsys_material_bind(Material* material, CommandList cmd);

	ShaderLibrary* matsys_material_shader_get(Material* material);
	
}