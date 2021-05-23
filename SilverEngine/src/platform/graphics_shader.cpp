#include "defines.h"

#include "graphics_internal.h"

// TEMP
#include <sstream>

namespace sv {

    bool graphics_shader_initialize()
    {
	return true;
    }

    bool graphics_shader_close()
    {
	return true;
    }

    inline std::string graphics_shader_random_path()
    {
	static u32 seed = 0u;
	u32 random = math_random_u32(seed);

	seed += 100;
	std::string filePath = "$system/" + std::to_string(random);

	return filePath;
    }

    bool graphics_shader_compile_string(const ShaderCompileDesc* desc, const char* str, u32 size, RawList& data)
    {
	std::string filepath = graphics_shader_random_path();

	bool res = file_write_text(filepath.c_str(), str, size, false);
	if (!res) return false;

	SV_CHECK(graphics_shader_compile_file(desc, filepath.c_str(), data));
		
	SV_CHECK(file_remove(filepath.c_str()));

	return res;
    }

    constexpr size_t BAT_SIZE = 500u;

    SV_INLINE static void append_bat(char* bat, size_t& offset, const char* src)
    {
	size_t size = strlen(src);
	if (size + offset > BAT_SIZE) {
	    SV_LOG_ERROR("Shader compiler batch string overflow");
	    SV_ASSERT(0);
	    return;
	}

	memcpy((void*)(bat + offset), src, size);
	offset += size;
    }
    
    bool graphics_shader_compile_file(const ShaderCompileDesc* desc, const char* srcPath, RawList& data)
    {
	std::string filePath = graphics_shader_random_path();

	std::stringstream bat;

	// .exe path
	bat << "system\\bin\\dxc.exe ";
	
	// API Specific
	switch (desc->api)
	{
	case GraphicsAPI_Vulkan:
	{
	    bat << "-spirv ";

	    // Shift resources
	    u32 shift = GraphicsLimit_Sampler;
	    bat << "-fvk-t-shift " << shift << " all ";
	    shift += GraphicsLimit_GPUImage;
	    bat << "-fvk-b-shift " << shift << " all ";
	    shift += GraphicsLimit_ConstantBuffer;
	    bat << "-fvk-u-shift " << shift << " all ";
	}
	break;
	}

	// Target
	bat << "-T ";
	switch (desc->shaderType)
	{
	case ShaderType_Vertex:
	    bat << "vs_";
	    break;
	case ShaderType_Pixel:
	    bat << "ps_";
	    break;
	case ShaderType_Geometry:
	    bat << "gs_";
	    break;
	}

	bat << desc->majorVersion << '_' << desc->minorVersion << ' ';

	// Entry point
	bat << "-E " << desc->entryPoint << ' ';

	// Optimization level
	bat << "-O3 ";

	// Include path
	{
	    const char* includepath = "system/shaders";
	    bat << "-I " << includepath << ' ';

	}

	// API Macro
	switch (desc->api)
	{
	case sv::GraphicsAPI_Vulkan:
	    bat << "-D " << "SV_API_VULKAN ";
	    break;
	}

	// Shader Macro

	switch (desc->shaderType)
	{
	case sv::ShaderType_Vertex:
	    bat << "-D " << "SV_VERTEX_SHADER ";
	    break;
	case sv::ShaderType_Pixel:
	    bat << "-D " << "SV_PIXEL_SHADER ";
	    break;
	case sv::ShaderType_Geometry:
	    bat << "-D " << "SV_GEOMETRY_SHADER ";
	    break;
	case sv::ShaderType_Hull:
	    bat << "-D " << "SV_HULL_SHADER ";
	    break;
	case sv::ShaderType_Domain:
	    bat << "-D " << "SV_DOMAIN_SHADER ";
	    break;
	case sv::ShaderType_Compute:
	    bat << "-D " << "SV_COMPUTE_SHADER ";
	    break;
	}

	// User Macro
	for (auto it = desc->macros.begin(); it != desc->macros.end(); ++it) {
	    const char* name = it->name;
	    const char* value = it->value;

	    if (strlen(name) == 0u) continue;

	    bat << "-D " << name;

	    if (strlen(value) != 0) {
		bat << '=' << value;
	    }

	    bat << ' ';
	}

	// Input - Output
	bat << srcPath + 1u << " -Fo ";
	bat << filePath.c_str() + 1u;

	bat << " 2> system\\shader_log.txt ";
	
	// Execute
	system(bat.str().c_str());

	// Read from file
	{
	    if (!file_read_binary(filePath.c_str(), data)) 
		return false;
	}

	// Remove tem file
	if (!file_remove(filePath.c_str()))
	{
	    SV_LOG_ERROR("Can't remove the shader temporal file at '%s'", filePath.c_str());
	}

	return true;
    }

    bool graphics_shader_compile_fastbin_from_string(const char* name, ShaderType shaderType, Shader** pShader, const char* src, bool alwaisCompile)
    {
	RawList data;
	size_t hash = hash_string(name);
	hash_combine(hash, shaderType);

	ShaderDesc desc;
	desc.shaderType = shaderType;

#if SV_GFX
	if (alwaisCompile || !bin_read(hash, data, true)) {
#else
	    if (!bin_read(hash, data, true)) {
#endif
			
		ShaderCompileDesc c;
		c.api = graphics_api_get();
		c.entryPoint = "main";
		c.majorVersion = 6u;
		c.minorVersion = 0u;
		c.shaderType = shaderType;

		SV_CHECK(graphics_shader_compile_string(&c, src, u32(strlen(src)), data));
		SV_CHECK(bin_write(hash, data.data(), u32(data.size()), true));

		SV_LOG_INFO("Shader Compiled: '%s'", name);
	    }

	    desc.binDataSize = data.size();
	    desc.pBinData = data.data();
	    return graphics_shader_create(&desc, pShader);
	}

	bool graphics_shader_compile_fastbin_from_file(const char* name, ShaderType shaderType, Shader** pShader, const char* filePath, bool alwaisCompile)
	{
	    RawList data;
	    size_t hash = hash_string(name);
	    hash_combine(hash, shaderType);

	    ShaderDesc desc;
	    desc.shaderType = shaderType;

#if SV_GFX
	    if (alwaisCompile || !bin_read(hash, data, true)) {
#else
		if (!bin_read(hash, data, true)) {
#endif
		    char* str;
		    size_t str_size;
		    if (!file_read_text(filePath, &str, &str_size)) {
			SV_LOG_ERROR("Shader source not found: %s", filePath);
			return false;
		    }

		    ShaderCompileDesc c;
		    c.api = graphics_api_get();
		    c.entryPoint = "main";
		    c.majorVersion = 6u;
		    c.minorVersion = 0u;
		    c.shaderType = shaderType;

		    if (!graphics_shader_compile_string(&c, str, u32(strlen(str)), data)) {
			SV_LOG_ERROR("Can't compile the shader '%s'", filePath);
			return false;
		    }

		    SV_FREE_MEMORY(str);

		    SV_CHECK(bin_write(hash, data.data(), u32(data.size()), true));

		    SV_LOG_INFO("Shader Compiled: '%s'", name);
		}

		desc.binDataSize = data.size();
		desc.pBinData = data.data();
		return graphics_shader_create(&desc, pShader);
	    }

	}




	/*

	  std::string filePath = graphics_shader_random_path();

	  size_t offset = 0u;
	  char bat[BAT_SIZE];
	  char aux[20];

	  // .exe path
	  append_bat(bat, offset, "system\\compilers\\dxc.exe ");
		
	  // API Specific
	  switch (desc->api)
	  {
	  case GraphicsAPI_Vulkan:
	  {
	  append_bat(bat, offset, "-spirv ");

	  // Shift resources
	  u32 shift = GraphicsLimit_Sampler;
	  append_bat(bat, offset, "-fvk-t-shift ");
	  sprintf(aux, "%u", shift);
	  append_bat(bat, offset, aux);
	  append_bat(bat, offset, " all ");
	    
	  shift += GraphicsLimit_GPUImage;
	  append_bat(bat, offset, "-fvk-b-shift ");
	  sprintf(aux, "%u", shift);
	  append_bat(bat, offset, aux);
	  append_bat(bat, offset, " all ");

	  shift += GraphicsLimit_ConstantBuffer;
	  append_bat(bat, offset, "-fvk-u-shift ");
	  sprintf(aux, "%u", shift);
	  append_bat(bat, offset, aux);
	  append_bat(bat, offset, " all ");
	  }
	  break;
	  }

	  // Target
	  append_bat(bat, offset, "-T ");
	  switch (desc->shaderType)
	  {
	  case ShaderType_Vertex:
	  append_bat(bat, offset, "vs_");
	  break;
	  case ShaderType_Pixel:
	  append_bat(bat, offset, "ps_");
	  break;
	  case ShaderType_Geometry:
	  append_bat(bat, offset, "gs_");
	  break;
	  }

	  sprintf(aux, "%u", desc->majorVersion);
	  append_bat(bat, offset, aux);

	  append_bat(bat, offset, "_");
	
	  sprintf(aux, "%u", desc->minorVersion);
	  append_bat(bat, offset, aux);

	  append_bat(bat, offset, " ");

	  // Entry point
	  append_bat(bat, offset, "-E ");
	  append_bat(bat, offset, desc->entryPoint);
	  append_bat(bat, offset, " ");

	  // Optimization level
	  append_bat(bat, offset, "-O3 ");

	  // Include path
	  append_bat(bat, offset, "-I system/shaders ");

	  // API Macro
	  switch (desc->api)
	  {
	  case sv::GraphicsAPI_Vulkan:
	  append_bat(bat, offset, "-D SV_API_VULKAN");
	  break;
	  }

	  // Shader Macro

	  switch (desc->shaderType)
	  {
	  case sv::ShaderType_Vertex:
	  append_bat(bat, offset, "-D SV_VERTEX_SHADER ");
	  break;
	  case sv::ShaderType_Pixel:
	  append_bat(bat, offset, "-D SV_PIXEL_SHADER ");
	  break;
	  case sv::ShaderType_Geometry:
	  append_bat(bat, offset, "-D SV_GEOMETRY_SHADER ");
	  break;
	  case sv::ShaderType_Hull:
	  append_bat(bat, offset, "-D SV_HULL_SHADER ");
	  break;
	  case sv::ShaderType_Domain:
	  append_bat(bat, offset, "-D SV_DOMAIN_SHADER ");
	  break;
	  case sv::ShaderType_Compute:
	  append_bat(bat, offset, "-D SV_COMPUTE_SHADER ");
	  break;
	  }

	  // User Macro
	  for (auto it = desc->macros.begin(); it != desc->macros.end(); ++it) {

	  const char* name = it->first;
	  const char* value = it->second;

	  if (strlen(name) == 0u) continue;

	  append_bat(bat, offset, "-D ");
	  append_bat(bat, offset, name);

	  if (strlen(value) != 0) {
	  append_bat(bat, offset, "=");
	  append_bat(bat, offset, value);
	  }

	  append_bat(bat, offset, " ");
	  }

	  // Input - Output
	  append_bat(bat, offset, srcPath);
	  append_bat(bat, offset, " -Fo ");
	  append_bat(bat, offset, filePath.c_str());
	
	  append_bat(bat, offset, "> wtf.txt 2> wtfe.txt");
	
	  SV_ASSERT(offset != BAT_SIZE);
	  bat[offset] = '\0';
	
	  // Execute
	  system(bat);
	*/
