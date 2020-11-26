#include "core.h"

#include "material_system_internal.h"
#include "utils/io.h"

namespace sv {

	enum ShaderTag : ui32 {

		ShaderTag_Null,
		ShaderTag_Unknown,
		ShaderTag_Name,
		ShaderTag_Type,
		ShaderTag_Begin,
		ShaderTag_End,

	};

	struct ShaderDefine {
		ShaderTag tag;
		std::string value;
	};

	struct SubShaderIntermediate {
		SubShaderID ID;
		std::vector<ui8> data;
	};

	void decomposeDefine(const char* line, ui32 size, ShaderDefine& define)
	{
		if (*line != '#') {
			define.tag = ShaderTag_Null;
			define.value.clear();
			return;
		}

		++line;
		const char* valuePtr = nullptr;

		if (line[0] == 'n' && line[1] == 'a' && line[2] == 'm' && line[3] == 'e' && line[4] == ' ') { define.tag = ShaderTag_Name; valuePtr = line + 5; }
		else if (line[0] == 't' && line[1] == 'y' && line[2] == 'p' && line[3] == 'e' && line[4] == ' ') { define.tag = ShaderTag_Type; valuePtr = line + 5; }
		else if (line[0] == 'b' && line[1] == 'e' && line[2] == 'g' && line[3] == 'i' && line[4] == 'n' && line[5] == ' ') { define.tag = ShaderTag_Begin; valuePtr = line + 6; }
		else if (line[0] == 'e' && line[1] == 'n' && line[2] == 'd') { define.tag = ShaderTag_End; }
		else { define.tag = ShaderTag_Unknown; valuePtr = line - 1u; }

		if (valuePtr) {
			size_t valueSize = size - (valuePtr - line) - 1u;
			define.value.resize(valueSize);
			memcpy(define.value.data(), valuePtr, valueSize);

			// Remove control chars
			for (auto it = define.value.begin(); it != define.value.end(); ) {
				if (*it == '\r') {
					it = define.value.erase(it);
				}
				else ++it;
			}
		}
	}

	bool getLineSize(const char* src, ui32& size)
	{
		const char* it = src;
		while (*it != '\n' && *it != '\0') ++it;

		size = ui32(it - src);
		return *it != '\0';
	}

	Result matsys_shaderlibrary_compile(ShaderLibrary_internal& lib, const char* src)
	{
		const char* line = src;
		ui32 lineSize;
		ShaderDefine define;

		std::string name;
		ShaderLibraryType_internal* type = nullptr;

		const char* inShader = nullptr;
		SubShaderID subShaderID = ui32_max;
		std::vector<SubShaderIntermediate> intermediate;

		while (true) {

			bool brk = !getLineSize(line, lineSize);

			if (brk && lineSize < 1u) break;

			decomposeDefine(line, lineSize, define);

			if (define.tag != ShaderTag_Null) {

				// Process shader define
				if (inShader) {

					bool end = false;

					switch (define.tag)
					{
					case ShaderTag_End:
						end = true;
						break;
					}

					// End Compilation
					if (end) {

						SubShaderRegister& shaderReg = type->subShaderRegisters[subShaderID];

						ShaderCompileDesc desc;
						desc.api = graphics_api_get();
						desc.entryPoint = "main";
						desc.majorVersion = 6u;
						desc.minorVersion = 0u;
						desc.shaderType = shaderReg.type;

						for (SubShaderIntermediate& i : intermediate) {
							if (i.ID == subShaderID) {
								SV_LOG_ERROR("Duplicated sub shader definition '%s'", shaderReg.name.c_str());
								return Result_CompileError;
							}
						}

						SubShaderIntermediate& inter = intermediate.emplace_back();
						inter.ID = subShaderID;

						std::string shaderSrc;
						shaderSrc = "#include \"core.hlsl\"\n";

						if (shaderReg.preLibName.size()) {
							shaderSrc += "#include \"";
							shaderSrc += shaderReg.preLibName;
							shaderSrc += ".hlsl\"\n";
						}

						size_t beginSize = shaderSrc.size();

						const char* beginSrc = inShader;
						const char* endSrc = line - 1u;

						shaderSrc.resize(beginSize + endSrc - beginSrc);
						memcpy(shaderSrc.data() + beginSize, beginSrc, shaderSrc.size() - beginSize);

						if (shaderReg.postLibName.size()) {
							shaderSrc += "\n#include \"";
							shaderSrc += shaderReg.postLibName;
							shaderSrc += ".hlsl\"\n";
						}

						svCheck(graphics_shader_compile_string(&desc, shaderSrc.c_str(), ui32(strlen(shaderSrc.c_str())), inter.data));
						inShader = nullptr;
					}
				}
				else {
					switch (define.tag)
					{
					case ShaderTag_Name:
						name = std::move(define.value);
						break;

					case ShaderTag_Type:
					{
						if (type) {
							SV_LOG_ERROR("Type already defined");
							return Result_CompileError;
						}

						type = typeFind(define.value.c_str());
						if (type == nullptr) {
							SV_LOG_ERROR("Invalid shader type %s", define.value.c_str());
							return Result_CompileError;
						}
					}break;

					case ShaderTag_Begin:
					{
						if (type == nullptr) {
							SV_LOG_ERROR("Type undefined");
							return Result_CompileError;
						}

						subShaderID = type->findSubShaderID(define.value.c_str());
						if (subShaderID == ui32_max) {
							SV_LOG_ERROR("Sub shader '%s' not found", define.value.c_str());
						}
						else inShader = line + lineSize + 1u;
					}break;

					case ShaderTag_End:
						SV_LOG_ERROR("Can't call end without calling begin");
						break;

					case ShaderTag_Unknown:
					default:
					{
						std::string defStr;
						defStr.resize(lineSize + 1u);
						memcpy(defStr.data(), line, lineSize);
						defStr.back() = '\0';
						SV_LOG_INFO("Unknown shader tag '%s'", defStr.c_str());
						break;
					}
					}
				}
			}

			if (brk) break;
			line += lineSize + 1u;
		}

		// Check some errors
		if (type == nullptr) {
			SV_LOG_ERROR("Unknown shader type");
			return Result_CompileError;
		}
		if (name.empty()) {
			SV_LOG_INFO("Shader must have name");
			return Result_CompileError;
		}

		// Asign subshaders
		for (SubShaderID i = 0u; i < type->subShaderCount; ++i) {

			SubShaderRegister& reg = type->subShaderRegisters[i];
			SubShaderIntermediate* pInter = nullptr;

			for (SubShaderIntermediate& inter : intermediate) {
				if (inter.ID == i) {
					pInter = &inter;
					break;
				}
			}

			if (pInter == nullptr) {

				if (reg.defaultShader == nullptr) {
					SV_LOG_ERROR("Sub Shader '%s' not found", reg.name.c_str());
					return Result_CompileError;
				}
				else lib.subShaders[i].shader = reg.defaultShader;
			}

		}

		// Create shaders
		{
			ShaderDesc desc;

			for (SubShaderIntermediate& inter : intermediate) {

				desc.shaderType = type->subShaderRegisters[inter.ID].type;
				desc.binDataSize = inter.data.size();
				desc.pBinData = inter.data.data();

				svCheck(graphics_shader_create(&desc, &lib.subShaders[inter.ID].shader));
			}
		}

		// Create shaderlibrary name and compute hashCode
		lib.name = std::move(name);
		lib.type = type;
		lib.nameHashCode = hash_string(lib.name.c_str());

		// Save bin
		{
			// Create Archive
			ArchiveO archive;
			archive << lib.name;
			archive << lib.type->name;

			archive << ui32(intermediate.size());

			for (SubShaderIntermediate& inter : intermediate) {
				archive << inter.ID;
				archive << inter.data;
			}

			// Compute hash and save it
			size_t hashCode = lib.nameHashCode;
			hash_combine(hashCode, graphics_api_get());

			Result binResult = bin_write(hashCode, archive);
			if (result_fail(binResult)) {
				SV_LOG_INFO("Can't generate bin data, error code = %u", binResult);
				return binResult;
			}
		}

		return Result_Success;
	}

}