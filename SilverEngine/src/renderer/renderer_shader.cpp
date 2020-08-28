#include "core.h"

#include "renderer_internal.h"

namespace sv {

	bool renderer_shader_create(const char* name, ShaderType type, Shader& shader)
	{
		GraphicsAPI api = graphics_api_get();

		// Create binPath string
		std::string binPath = "bin/shaders/";

#ifdef SV_SRC_PATH
		binPath = SV_SRC_PATH + binPath;
#endif

		switch (api)
		{
		case sv::GraphicsAPI_Vulkan:
			binPath += "vk_";
			break;
		}

		switch (type)
		{
		case sv::ShaderType_Vertex:
			binPath += "vs_";
			break;
		case sv::ShaderType_Pixel:
			binPath += "ps_";
			break;
		case sv::ShaderType_Geometry:
			binPath += "gs_";
			break;
		case sv::ShaderType_Hull:
			binPath += "hs_";
			break;
		case sv::ShaderType_Domain:
			binPath += "ds_";
			break;
		case sv::ShaderType_Compute:
			binPath += "cs_";
			break;
		}

		binPath += name;

		switch (api)
		{
		case sv::GraphicsAPI_Vulkan:
			binPath += ".spv";
			break;
		}
		
		// Find binary data
		std::ifstream input;
		input.open(binPath, std::ios::binary | std::ios::ate);

		std::vector<ui8> data;

		// Get binary data
		if (input.is_open()) {
			size_t size = input.tellg();
			input.seekg(0u);
			data.resize(size);
			input.read((char*)data.data(), size);
		}
		// Compile hlsl file
		else {

			// Create src path string
			std::string srcPath = "shaders/";
			srcPath += name;
			srcPath += ".hlsl";

#ifdef SV_SRC_PATH
			srcPath = SV_SRC_PATH + srcPath;
#endif

			ShaderCompileDesc compileDesc;
			compileDesc.api = api;
			compileDesc.shaderType = type;
			compileDesc.majorVersion = 6u;
			compileDesc.minorVersion = 0u;
			compileDesc.entryPoint = "main";
			
			// API Macro
			switch (api)
			{
			case sv::GraphicsAPI_Vulkan:
				compileDesc.macros.push_back(std::make_pair("SV_API_VULKAN", ""));
				break;
			}
			
			// Shader Macro

			switch (type)
			{
			case sv::ShaderType_Vertex:
				compileDesc.macros.push_back(std::make_pair("SV_SHADER_TYPE_VERTEX", ""));
				break;
			case sv::ShaderType_Pixel:
				compileDesc.macros.push_back(std::make_pair("SV_SHADER_TYPE_PIXEL", ""));
				break;
			case sv::ShaderType_Geometry:
				compileDesc.macros.push_back(std::make_pair("SV_SHADER_TYPE_GEOMETRY", ""));
				break;
			case sv::ShaderType_Hull:
				compileDesc.macros.push_back(std::make_pair("SV_SHADER_TYPE_HULL", ""));
				break;
			case sv::ShaderType_Domain:
				compileDesc.macros.push_back(std::make_pair("SV_SHADER_TYPE_DOMAIN", ""));
				break;
			case sv::ShaderType_Compute:
				compileDesc.macros.push_back(std::make_pair("SV_SHADER_TYPE_COMPUTE", ""));
				break;
			}

			// Lighting Macro
			std::string lightCountStr = std::to_string(SV_REND_FORWARD_LIGHT_COUNT);
			compileDesc.macros.push_back(std::make_pair("SV_LIGHT_COUNT", lightCountStr.c_str()));

			// Compile
			const char* shaderName;
			switch (type)
			{
			case sv::ShaderType_Vertex:
				shaderName = "Vertex Shader";
				break;
			case sv::ShaderType_Pixel:
				shaderName = "Pixel Shader";
				break;
			case sv::ShaderType_Geometry:
				shaderName = "Geometry Shader";
				break;
			case sv::ShaderType_Hull:
				shaderName = "Hull Shader";
				break;
			case sv::ShaderType_Domain:
				shaderName = "Domain Shader";
				break;
			case sv::ShaderType_Compute:
				shaderName = "Vertex Compute";
				break;
			default:
				shaderName = "Unknown Shader";
				break;
			}

			if (graphics_shader_compile_file(&compileDesc, srcPath.c_str(), binPath.c_str())) {

				log_info("%s (%s) successfully compiled", name, shaderName);
			}
			else {
				log_error("%s (%s) failed compiling", name, shaderName);
			}

			// Check if exists and read bin data
			input.open(binPath.c_str(), std::ios::binary | std::ios::ate);

			svCheck(input.is_open());

			size_t size = input.tellg();
			input.seekg(0u);
			data.resize(size);
			input.read((char*)data.data(), size);
		}

		input.close();

		ShaderDesc desc;
		desc.pBinData = data.data();
		desc.binDataSize = data.size();
		desc.shaderType = type;
		
		svCheck(graphics_shader_create(&desc, shader));
		return true;
	}

}