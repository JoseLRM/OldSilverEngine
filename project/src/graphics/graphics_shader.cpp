#include "SilverEngine/core.h"

#include "graphics_internal.h"

namespace sv {

	Result graphics_shader_initialize()
	{
		return Result_Success;
	}

	Result graphics_shader_close()
	{
		return Result_Success;
	}

	inline std::string graphics_shader_random_path()
	{
		static u32 seed = 0u;
		u32 random = math_random_u32(seed);

		seed += 100;
		std::string filePath = std::to_string(random);

		return filePath;
	}

	Result graphics_shader_compile_string(const ShaderCompileDesc* desc, const char* str, u32 size, std::vector<u8>& data)
	{
		std::string filePath = graphics_shader_random_path();

		FileO tempFile;
		svCheck(tempFile.open(filePath.c_str()));

		if (!tempFile.isOpen()) return Result_UnknownError;

		tempFile.write((u8*)str, size);
		tempFile.close();

		Result res = graphics_shader_compile_file(desc, filePath.c_str(), data);

		svCheck(file_remove(filePath.c_str()));

		return res;
	}

	Result graphics_shader_compile_file(const ShaderCompileDesc* desc, const char* srcPath, std::vector<u8>& data)
	{
		std::string filePath = graphics_shader_random_path();

		std::stringstream bat;

		// .exe path
		{
#ifdef SV_RES_PATH
			std::string resPath = SV_RES_PATH;
			for (char& c : resPath) if (c == '/') c = '\\';
			bat << resPath;
#endif
			bat << "library\\compilers\\dxc.exe ";
		}

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
			const char* includePath = "library/shader_utils";

#ifdef SV_RES_PATH
			std::string includePathStr = SV_RES_PATH;
			includePathStr += includePath;
			includePath = includePathStr.c_str();
#endif

			bat << "-I " << includePath << ' ';

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
			bat << "-D " << "SV_SHADER_TYPE_VERTEX ";
			break;
		case sv::ShaderType_Pixel:
			bat << "-D " << "SV_SHADER_TYPE_PIXEL ";
			break;
		case sv::ShaderType_Geometry:
			bat << "-D " << "SV_SHADER_TYPE_GEOMETRY ";
			break;
		case sv::ShaderType_Hull:
			bat << "-D " << "SV_SHADER_TYPE_HULL ";
			break;
		case sv::ShaderType_Domain:
			bat << "-D " << "SV_SHADER_TYPE_DOMAIN ";
			break;
		case sv::ShaderType_Compute:
			bat << "-D " << "SV_SHADER_TYPE_COMPUTE ";
			break;
		}

		// User Macro
		for (auto it = desc->macros.begin(); it != desc->macros.end(); ++it) {
			auto [name, value] = *it;

			if (strlen(name) == 0u) continue;

			bat << "-D " << name;

			if (strlen(value) != 0) {
				bat << '=' << value;
			}

			bat << ' ';
		}

		// Input - Output
#ifdef SV_RES_PATH
		bat << SV_RES_PATH;
#endif
		bat << srcPath << " -Fo ";
#ifdef SV_RES_PATH
		bat << SV_RES_PATH;
#endif
		bat << filePath;

		// Execute
		system(bat.str().c_str());

		// Read from file
		{
			if (result_fail(file_read_binary(filePath.c_str(), data))) 
				return Result_CompileError;
		}

		// Remove tem file
		svCheck(file_remove(filePath.c_str()));

		return Result_Success;
	}

	Result graphics_shader_compile_fastbin_from_string(const char* name, ShaderType shaderType, Shader** pShader, const char* src, bool alwaisCompile)
	{
		std::vector<u8> data;
		size_t hash = hash_string(name);

		ShaderDesc desc;
		desc.shaderType = shaderType;

#ifdef SV_ENABLE_GFX_VALIDATION
		if (alwaisCompile || result_fail(bin_read(hash, data))) {
#else
		if (result_fail(bin_read(hash, data))) {
#endif
			
			ShaderCompileDesc c;
			c.api = graphics_api_get();
			c.entryPoint = "main";
			c.majorVersion = 6u;
			c.minorVersion = 0u;
			c.shaderType = shaderType;

			svCheck(graphics_shader_compile_string(&c, src, u32(strlen(src)), data));
			svCheck(bin_write(hash, data.data(), u32(data.size())));
		}

		desc.binDataSize = data.size();
		desc.pBinData = data.data();
		return graphics_shader_create(&desc, pShader);
	}

	Result graphics_shader_compile_fastbin_from_file(const char* name, ShaderType shaderType, Shader** pShader, const char* filePath, bool alwaisCompile)
	{
		std::vector<u8> data;
		size_t hash = hash_string(name);

		ShaderDesc desc;
		desc.shaderType = shaderType;

#ifdef SV_ENABLE_GFX_VALIDATION
		if (alwaisCompile || result_fail(bin_read(hash, data))) {
#else
		if (result_fail(bin_read(hash, data))) {
#endif
			std::string str;
			svCheck(file_read_text(filePath, str));

			ShaderCompileDesc c;
			c.api = graphics_api_get();
			c.entryPoint = "main";
			c.majorVersion = 6u;
			c.minorVersion = 0u;
			c.shaderType = shaderType;

			svCheck(graphics_shader_compile_string(&c, str.data(), u32(strlen(str.data())), data));
			svCheck(bin_write(hash, data.data(), u32(data.size())));
		}

		desc.binDataSize = data.size();
		desc.pBinData = data.data();
		return graphics_shader_create(&desc, pShader);
	}

}