#include "core.h"

#include "graphics_internal.h"

//#include <d3d11.h>
//#include <dxcapi.h>
//#include <wrl/client.h>
//using namespace Microsoft::WRL;
//#define dxCheck(x) if (FAILED(x)) return false

namespace sv {

	//static ComPtr<IDxcLibrary> g_Library;
	//static ComPtr<IDxcCompiler> g_Compiler;

	Result graphics_shader_initialize()
	{
		//dxCheck(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&g_Library)));
		//dxCheck(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&g_Compiler)));

		return Result_Success;
	}

	Result graphics_shader_close()
	{
		//g_Library.Reset();
		//g_Compiler.Reset();

		return Result_Success;
	}

	Result graphics_shader_compile_string(const ShaderCompileDesc* desc, const char* str, ui32 size, std::vector<ui8>& data)
	{
		//TODO:
		svLogError("TODO->graphics_shader_compile_from_string");
		return Result_UnknownError;
		/*
		ComPtr<IDxcBlobEncoding> srcBlob;
		dxCheck(g_Library->CreateBlobWithEncodingFromPinned(str, size, CP_UTF8, srcBlob.GetAddressOf()));
		
		// Target
		std::wstringstream target;
		switch (desc->shaderType)
		{
		case ShaderType_Vertex:
			target << L"vs_";
			break;
		case ShaderType_Pixel:
			target << L"ps_";
			break;
		case ShaderType_Geometry:
			target << L"gs_";
			break;
		}
		
		target << desc->majorVersion << L'_' << desc->minorVersion << L' ';
		
		// Macros
		std::vector<DxcDefine> defines(desc->macros.size() + 2u);
		
		ui32 i = 0u;
		for (; i < desc->macros.size(); ++i) {
			DxcDefine& def = defines[i];
			//def.Name = desc->macros[i].first;
			//def.Value = desc->macros[i].second;
		}
		
		switch (desc->shaderType)
		{
		case ShaderType_Vertex:
			defines[i++] = { L"SV_SHADER_TYPE_VERTEX", L"" };
			break;
		case ShaderType_Pixel:
			defines[i++] = { L"SV_SHADER_TYPE_PIXEL", L"" };
			break;
		case ShaderType_Geometry:
			defines[i++] = { L"SV_SHADER_TYPE_GEOMETRY", L"" };
			break;
		}
		
		switch (api)
		{
		case sv::GraphicsAPI_Vulkan:
			defines[i++] = { L"SV_API_VULKAN", L"" };
			break;
		}
		
		// Compile
		ComPtr<IDxcOperationResult> result;
		HRESULT hr = -1;
		
		switch (api)
		{
		case sv::GraphicsAPI_Vulkan:
		{
			std::wstring shiftTRes = L"-fvk-t-shift ";
			shiftTRes += std::to_wstring(SV_GFX_SAMPLER_COUNT);
			shiftTRes += L" all";
		
			std::wstring shiftBRes = L"-fvk-b-shift ";
			shiftBRes += std::to_wstring(SV_GFX_SAMPLER_COUNT + SV_GFX_IMAGE_COUNT);
			shiftBRes += L" all";
		
			const wchar* args[] = {
				L"-WX", // Warnings as errors
				L"-DENABLE_SPIRV_CODEGEN=ON",
				L"-spirv",
				//shiftTRes.c_str(),
				//shiftBRes.c_str(),
			};
		
			hr = g_Compiler->Compile(srcBlob.Get(), L"Nepe.hlsl", L"main", L"vs_6_0", args, sizeof(args) / sizeof(args[0]), defines.data(), ui32(defines.size()), nullptr, &result);
			break;
		}
		}
		
		// Get errors
		if (SUCCEEDED(hr)) {
			result->GetStatus(&hr);
		}
		
		if (FAILED(hr) && result) {
			ComPtr<IDxcBlobEncoding> errorsBlob;
			dxCheck(result->GetErrorBuffer(&errorsBlob));
		
			log_error("Shader compilation failed: %s", (const char*)errorsBlob->GetBufferPointer());
			return false;
		}
		
		// Get bin data
		ComPtr<IDxcBlob> dataBlob;
		dxCheck(result->GetResult(&dataBlob));
		
		data.resize(dataBlob->GetBufferSize());
		memcpy(data.data(), dataBlob->GetBufferPointer(), data.size());
		
		return true;
		*/
	}

	Result graphics_shader_compile_file(const ShaderCompileDesc* desc, const char* srcPath, const char* binPath)
	{
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
			ui32 shift = SV_GFX_SAMPLER_COUNT;
			bat << "-fvk-t-shift " << shift << " all ";
			shift += SV_GFX_IMAGE_COUNT;
			bat << "-fvk-b-shift " << shift << " all ";
			shift += SV_GFX_CONSTANT_BUFFER_COUNT;
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

		// Macros
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
		bat << srcPath << " -Fo " << binPath;

		// Execute
		system(bat.str().c_str());

		return Result_Success;
	}

}