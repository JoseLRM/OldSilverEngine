#include "core.h"

#include "material_system_internal.h"

#include "utils/io.h"

namespace sv {

	void matsys_shaderlibrary_textures_add(const ShaderInfo* info, ShaderType shaderType, std::vector<ShaderLibraryAttribute>& attributes, std::vector<ShaderIndices>& indices)
	{
		ui32 beginSize = attributes.size();

		const ShaderInfo::ResourceImage* src = info->images.data();
		const ShaderInfo::ResourceImage* endSrc = info->images.data() + info->images.size();

		while (src != endSrc) {

			const ShaderInfo::ResourceImage& attr = *src;

			// Find if exist
			ui32 i;
			for (i = 0; i < beginSize; ++i) {
				auto& a = attributes[i];
				if (string_equal(a.name.c_str(), a.name.size(), attr.name.c_str(), attr.name.size())) {
					break;
				}
			}

			// If not exist add new
			if (i == beginSize) {
				i = ui32(attributes.size());
				auto& a = attributes.emplace_back();
				a.name = attr.name;
				a.type = ShaderAttributeType_Texture;
				indices.emplace_back();
			}

			// Set texture slot
			indices[i].i[shaderType] = attr.bindingSlot;

			++src;
		}

	}

	void matsys_shaderlibrary_attributes_add(const ShaderInfo* info, ShaderType shaderType, std::vector<ShaderLibraryAttribute>& attributes, std::vector<ShaderIndices>& indices, ui32& cameraBinding, ui32& binding, ui32& size)
	{
		ui32 beginSize = attributes.size();

		// Find camera buffer
		cameraBinding = ui32_max;
		for (auto& m : info->constantBuffers)
		{
			if (strcmp(m.name.c_str(), "__Camera__") == 0) {
				cameraBinding = m.bindingSlot;
				break;
			}
		}

		// Find material buffer
		const ShaderInfo::ResourceBuffer* mat = nullptr;
		for (auto& m : info->constantBuffers)
		{
			if (strcmp(m.name.c_str(), "__Material__") == 0) {
				mat = &m;
				break;
			}
		}
		if (mat == nullptr) return;

		binding = mat->bindingSlot;
		size = mat->size;

		const ShaderAttribute* src = mat->attributes.data();
		const ShaderAttribute* endSrc = mat->attributes.data() + mat->attributes.size();

		while (src != endSrc) {

			const ShaderAttribute& attr = *src;

			// Find if exist
			ui32 i;
			for (i = 0; i < beginSize; ++i) {
				auto& a = attributes[i];
				if (a.type == attr.type && string_equal(a.name.c_str(), a.name.size(), attr.name.c_str(), attr.name.size())) {
					break;
				}
			}

			// If not exist add new
			if (i == beginSize) {
				i = ui32(attributes.size());
				auto& a = attributes.emplace_back();
				a.name = attr.name;
				a.type = attr.type;
				indices.emplace_back();
			}

			indices[i].i[shaderType] = attr.offset;

			++src;
		}

	}

	enum ShaderTag : ui32 {

		ShaderTag_Null,
		ShaderTag_Unknown,
		ShaderTag_Name,
		ShaderTag_Package,
		ShaderTag_VSbegin,
		ShaderTag_VSend,
		ShaderTag_PSbegin,
		ShaderTag_PSend,

	};

	struct ShaderDefine {
		ShaderTag tag;
		const char* pValue;
		ui32 valueSize;
	};

	ShaderDefine matsys_decompose_define(const char* line, ui32 size)
	{
		ShaderDefine def;
		if (*line != '#') return { ShaderTag_Null, nullptr, 0u };

		++line;

		if (line[0] == 'n' && line[1] == 'a' && line[2] == 'm' && line[3] == 'e' && line[4] == ' ') def = { ShaderTag_Name, line + 5 };
		else if (line[0] == 'p' && line[1] == 'a' && line[2] == 'c' && line[3] == 'k' && line[4] == 'a' && line[5] == 'g' && line[6] == 'e' && line[7] == ' ') def = { ShaderTag_Package, line + 8 };
		else if (line[0] == 'V' && line[1] == 'S' && line[2] == '_' && line[3] == 'b' && line[4] == 'e' && line[5] == 'g' && line[6] == 'i' && line[7] == 'n') def = { ShaderTag_VSbegin, nullptr };
		else if (line[0] == 'P' && line[1] == 'S' && line[2] == '_' && line[3] == 'b' && line[4] == 'e' && line[5] == 'g' && line[6] == 'i' && line[7] == 'n') def = { ShaderTag_PSbegin, nullptr };
		else if (line[0] == 'V' && line[1] == 'S' && line[2] == '_' && line[3] == 'e' && line[4] == 'n' && line[5] == 'd') def = { ShaderTag_VSend, nullptr };
		else if (line[0] == 'P' && line[1] == 'S' && line[2] == '_' && line[3] == 'e' && line[4] == 'n' && line[5] == 'd') def = { ShaderTag_PSend, nullptr };
		else def = { ShaderTag_Unknown, line - 1u };

		if (def.pValue)
			def.valueSize = size - (def.pValue - line) - 1;
		return def;
	}

	bool matsys_get_line_size(const char* src, ui32& size)
	{
		const char* it = src;
		while (*it != '\n' && *it != '\0') ++it;

		size = it - src;
		return *it != '\0';
	}

	Result matsys_shaderlibrary_compile(ShaderLibrary_internal& lib, const char* src)
	{
		const char* line = src;
		ui32 lineSize;

		std::string name;
		std::string package;

		const char* inShader = nullptr;
		ShaderType currentShader;

		std::vector<ui8> VSBin, PSBin, GSBin;

		while (true) {

			bool brk = !matsys_get_line_size(line, lineSize);

			if (brk && lineSize < 1u) break;

			ShaderDefine define = matsys_decompose_define(line, lineSize);

			if (define.tag != ShaderTag_Null) {

				// Process shader define
				if (inShader) {

					bool end = false;

					switch (define.tag)
					{
					case ShaderTag_VSend:
					case ShaderTag_PSend:
						if ((define.tag == ShaderTag_VSend && currentShader != ShaderType_Vertex) || (define.tag == ShaderTag_PSend && currentShader != ShaderType_Pixel)) {
							svLogCompile("Can't end a shader with diferent type");
							return Result_CompileError;
						}
						end = true;
						break;
					}

					// End Compilation
					if (end) {

						ShaderCompileDesc desc;
						desc.api = graphics_api_get();
						desc.entryPoint = "main";
						desc.majorVersion = 6u;
						desc.minorVersion = 0u;
						desc.shaderType = currentShader;

						std::vector<ui8>* pBin = nullptr;
						switch (currentShader)
						{
						case sv::ShaderType_Vertex:
							if (VSBin.size()) {
								svLogCompile("Can't have more than one vertex shader code");
								return Result_CompileError;
							}
							pBin = &VSBin;
							break;
						case sv::ShaderType_Pixel:
							if (PSBin.size()) {
								svLogCompile("Can't have more than one pixel shader code");
								return Result_CompileError;
							}
							pBin = &PSBin;
							break;
						case sv::ShaderType_Geometry:
							if (GSBin.size()) {
								svLogCompile("Can't have more than one geometry shader code");
								return Result_CompileError;
							}
							pBin = &GSBin;
							break;
						default:
							return Result_UnknownError;
							break;
						}

						std::string shaderSrc;
						const char* beginSrc = inShader;
						const char* endSrc = line - 1u;
						
						shaderSrc.resize(endSrc - beginSrc);
						memcpy(shaderSrc.data(), beginSrc, shaderSrc.size());

						svCheck(graphics_shader_compile_string(&desc, shaderSrc.c_str(), strlen(shaderSrc.c_str()), *pBin));
						inShader = nullptr;
					}
				}
				else {
					switch (define.tag)
					{
					case ShaderTag_Name:
						name.resize(define.valueSize);
						memcpy(name.data(), define.pValue, define.valueSize);
						break;

					case ShaderTag_Package:
						package.resize(define.valueSize);
						memcpy(package.data(), define.pValue, define.valueSize);
						break;

					case ShaderTag_VSbegin:
						inShader = line + lineSize + 1u;
						currentShader = ShaderType_Vertex;
						break;

					case ShaderTag_PSbegin:
						inShader = line + lineSize + 1u;
						currentShader = ShaderType_Pixel;
						break;

					case ShaderTag_VSend:
					case ShaderTag_PSend:
						svLogCompile("Can't call end without calling begin");
						break;

					case ShaderTag_Unknown:
					default:
					{
						std::string defStr;
						defStr.resize(lineSize + 1u);
						memcpy(defStr.data(), line, lineSize);
						defStr.back() = '\0';
						svLogCompile("Unknown shader tag '%s'", defStr.c_str());
						break;
					}
					}
				}
			}
			else if (!inShader) {
				std::string defStr;
				defStr.resize(lineSize + 1u);
				memcpy(defStr.data(), line, lineSize);
				defStr.back() = '\0';
				svLogCompile("Unknown line outside shader code '%s'", defStr.c_str());
				break;
			}

			if (brk) break;
			line += lineSize + 1u;
		}

		// Check some errors
		if (VSBin.empty()) {
			svLogCompile("Shader must have vertex shader code");
			return Result_CompileError;
		}
		if (name.empty()) {
			svLogCompile("Shader must have name");
			return Result_CompileError;
		}

		// Create shaders
		{
			ShaderDesc desc;

			// Vertex Shader
			desc.shaderType = ShaderType_Vertex;
			desc.pBinData = VSBin.data();
			desc.binDataSize = ui32(VSBin.size());
			svCheck(graphics_shader_create(&desc, &lib.vs));

			// Pixel Shader
			if (PSBin.size()) {
				desc.shaderType = ShaderType_Pixel;
				desc.pBinData = PSBin.data();
				desc.binDataSize = ui32(PSBin.size());
				svCheck(graphics_shader_create(&desc, &lib.ps));
			}

			// Geometry Shader
			if (GSBin.size()) {
				desc.shaderType = ShaderType_Geometry;
				desc.pBinData = GSBin.data();
				desc.binDataSize = ui32(GSBin.size());
				svCheck(graphics_shader_create(&desc, &lib.gs));
			}
		}

		// Create shaderlibrary name
		if (package.size()) {
			lib.name = package;
			if (lib.name.back() != '/')
				lib.name += '/';
		}
		lib.name += name;

		// Compute hascode
		lib.hashCode = hash_string(lib.name.c_str());

		// Save bin
		{
			ArchiveO archive;
			archive << lib.name;
			archive << VSBin;

			if (PSBin.empty()) archive << false;
			else archive << true << PSBin;

			if (GSBin.empty()) archive << false;
			else archive << true << GSBin;

			size_t hashCode = lib.hashCode;
			hash_combine(hashCode, graphics_api_get());

			Result binResult = bin_write(hashCode, archive);
			if (result_fail(binResult)) {
				svLogCompile("Can't generate bin data, error code = %u", binResult);
				return binResult;
			}
		}

		return Result_Success;
	}

	Result matsys_shaderlibrary_construct(ShaderLibrary_internal& lib)
	{
		const ShaderInfo* info;

		// Textures
		{
			// Add VertexShader textures
			info = graphics_shader_info_get(lib.vs);
			matsys_shaderlibrary_textures_add(info, ShaderType_Vertex, lib.attributes, lib.attributeIndices);

			// Add PixelShader textures
			if (lib.ps) {
				info = graphics_shader_info_get(lib.ps);
				matsys_shaderlibrary_textures_add(info, ShaderType_Pixel, lib.attributes, lib.attributeIndices);
			}

			// Add GeometryShader textures
			if (lib.gs) {
				info = graphics_shader_info_get(lib.gs);
				matsys_shaderlibrary_textures_add(info, ShaderType_Geometry, lib.attributes, lib.attributeIndices);
			}
		}

		lib.texturesCount = ui32(lib.attributes.size());

		// Buffer data
		{
			// Add VertexShader buffer data
			info = graphics_shader_info_get(lib.vs);
			matsys_shaderlibrary_attributes_add(info, ShaderType_Vertex, lib.attributes, lib.attributeIndices, lib.cameraBufferBinding.i[ShaderType_Vertex], lib.bufferBindigs[ShaderType_Vertex], lib.bufferSizes[ShaderType_Vertex]);

			// Add PixelShader buffer data
			if (lib.ps) {
				info = graphics_shader_info_get(lib.ps);
				matsys_shaderlibrary_attributes_add(info, ShaderType_Pixel, lib.attributes, lib.attributeIndices, lib.cameraBufferBinding.i[ShaderType_Pixel], lib.bufferBindigs[ShaderType_Pixel], lib.bufferSizes[ShaderType_Pixel]);
			}

			// Add GeometryShader buffer data
			if (lib.gs) {
				info = graphics_shader_info_get(lib.gs);
				matsys_shaderlibrary_attributes_add(info, ShaderType_Geometry, lib.attributes, lib.attributeIndices, lib.cameraBufferBinding.i[ShaderType_Geometry], lib.bufferBindigs[ShaderType_Geometry], lib.bufferSizes[ShaderType_Geometry]);
			}
		}

		// Sizes count
		lib.bufferSizesCount = 0u;
		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			lib.bufferSizesCount += lib.bufferSizes[i];
		}

		return Result_Success;
	}

}