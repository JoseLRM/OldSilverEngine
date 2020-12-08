#include "core.h"

#include "graphics_internal.h"
#include "utils/io.h"

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
		static ui32 seed = 0u;
		ui32 random = math_random(seed);

		seed += 100;
		std::string filePath = std::to_string(random);

#ifdef SV_RES_PATH
		filePath = SV_RES_PATH + filePath;
#endif

		return filePath;
	}

	Result graphics_shader_compile_string(const ShaderCompileDesc* desc, const char* str, ui32 size, std::vector<ui8>& data)
	{
		std::string filePath = graphics_shader_random_path();

		std::ofstream tempFile(filePath, std::ios::ate | std::ios::binary);
		if (!tempFile.is_open()) return Result_UnknownError;

		tempFile.write(str, size);
		tempFile.close();

		Result res = graphics_shader_compile_file(desc, filePath.c_str(), data);

		if (remove(filePath.c_str()) != 0) {
			return Result_UnknownError;
		}

		return res;
	}

	Result graphics_shader_compile_file(const ShaderCompileDesc* desc, const char* srcPath, std::vector<ui8>& data)
	{
		std::string filePath = graphics_shader_random_path();

		std::stringstream bat;

		// .exe path
		bat << "dxc.exe ";

		// API Specific
		switch (desc->api)
		{
		case GraphicsAPI_Vulkan:
		{
			bat << "-spirv ";

			// Shift resources
			ui32 shift = GraphicsLimit_Sampler;
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
		bat << srcPath << " -Fo " << filePath;

		// Execute
		system(bat.str().c_str());

		// Read from file
		{
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);
			if (!file.is_open()) return Result_CompileError;

			size_t size = file.tellg();
			file.seekg(0);
			size_t index = data.size();
			data.resize(index + size);
			file.read((char*)data.data() + index, size);

			file.close();
		}

		// Remove tem file
		if (remove(filePath.c_str()) != 0) return Result_UnknownError;

		return Result_Success;
	}

	Result graphics_shader_compile_fastbin(const char* name, ShaderType shaderType, Shader** pShader, const char* src)
	{
		std::vector<ui8> data;
		size_t hash = hash_string(name);

		ShaderDesc desc;
		desc.shaderType = shaderType;

		if (result_fail(bin_read(hash, data))) {
			
			ShaderCompileDesc c;
			c.api = graphics_api_get();
			c.entryPoint = "main";
			c.majorVersion = 6u;
			c.minorVersion = 0u;
			c.shaderType = shaderType;

			svCheck(graphics_shader_compile_string(&c, src, ui32(strlen(src)), data));
			svCheck(bin_write(hash, data.data(), ui32(data.size())));
		}

		desc.binDataSize = data.size();
		desc.pBinData = data.data();
		return graphics_shader_create(&desc, pShader);
	}

}