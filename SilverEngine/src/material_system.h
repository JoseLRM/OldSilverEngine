#pragma once

#include "graphics.h"

namespace sv {

	SV_DEFINE_HANDLE(ShaderLibrary);
	SV_DEFINE_HANDLE(Material);

	struct ShaderLibraryDesc {
		Shader* vertexShader;
		Shader* pixelShader;
		Shader* geometryShader;
	};

	struct MaterialDesc {
		ShaderLibrary*	shaderLibrary;
		bool			dynamic;
	};

	// Shader Library

	Result matsys_shaderlibrary_create(const ShaderLibraryDesc* desc, ShaderLibrary** pShaderLibrary);
	Result matsys_shaderlibrary_recreate(const ShaderLibraryDesc* desc, ShaderLibrary* shaderLibrary);
	Result matsys_shaderlibrary_destroy(ShaderLibrary* shaderLibrary);

	const std::vector<ShaderAttribute>& matsys_shaderlibrary_attributes_get(ShaderLibrary* shaderLibrary);
	ui32 matsys_shaderlibrary_texture_count(ShaderLibrary* shaderLibrary);
	Shader* matsys_shaderlibrary_shader_get(ShaderLibrary* shaderLibrary, ShaderType shaderType);

	void matsys_shaderlibrary_bind(ShaderLibrary* shaderLibrary, CommandList cmd);

	// Material

	Result matsys_material_create(const MaterialDesc* desc, Material** pMaterial);
	Result matsys_material_recreate(ShaderLibrary* shaderLibrary, Material* material);
	Result matsys_material_destroy(Material* material);

	/*
		Set material attribute value.
		type parameter can be Unknown but if specifying the type may perform optimizations
	*/
	Result matsys_material_set(Material* material, const char* name, ShaderAttributeType type, const void* data);
	/*
		Get material attribute value.
		type parameter can be Unknown but if specifying the type may perform optimizations
	*/
	Result matsys_material_get(Material* material, const char* name, ShaderAttributeType type, void* data);

	void matsys_material_bind(Material* material, CommandList cmd);
	void matsys_material_update(Material* material, CommandList cmd);

	ShaderLibrary* matsys_material_shader_get(Material* material);
	
}