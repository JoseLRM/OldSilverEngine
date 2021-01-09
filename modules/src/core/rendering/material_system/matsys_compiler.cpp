#include "core.h"

#include "material_system_internal.h"
#include "core/utils/io.h"

namespace sv {

	enum ShaderTag : u32 {

		ShaderTag_Null,
		ShaderTag_Unknown,
		ShaderTag_Name,
		ShaderTag_Type,
		ShaderTag_Begin,
		ShaderTag_End,
		ShaderTag_UserBlock,
		ShaderTag_ShaderType,

	};

	struct ShaderDefine {
		ShaderTag tag;
		std::string value;
	};

	struct UserBlock {
		std::string name;
		std::string src;
	};

	void decomposeDefine(const char* line, u32 size, ShaderDefine& define)
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
		else if (line[0] == 'u' && line[1] == 's' && line[2] == 'e' && line[3] == 'r' && line[4] == 'b' && line[5] == 'l' && line[6] == 'o' && line[7] == 'c' && line[8] == 'k' && line[9] == ' ') { define.tag = ShaderTag_UserBlock; valuePtr = line + 10; }
		else if (line[0] == 's' && line[1] == 'h' && line[2] == 'a' && line[3] == 'd' && line[4] == 'e' && line[5] == 'r' && line[6] == 't' && line[7] == 'y' && line[8] == 'p' && line[9] == 'e' && line[10] == ' ') { define.tag = ShaderTag_ShaderType; valuePtr = line + 11; }
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

	bool getLineSize(const char* src, u32& size)
	{
		const char* it = src;
		while (*it != '\n' && *it != '\0') ++it;

		size = u32(it - src);
		return *it != '\0';
	}

	Result matsys_shaderlibrary_compile(ShaderLibrary_internal& lib, const char* src)
	{
		const char* line = src;
		u32 lineSize;
		ShaderDefine define;

		std::string name;
		ShaderLibraryType_internal* type = nullptr;

		const char* inBlock = nullptr;
		std::string currentBlockName;
		std::vector<UserBlock> userBlocks;

		while (true) {

			bool brk = !getLineSize(line, lineSize);

			if (brk && lineSize < 1u) break;

			decomposeDefine(line, lineSize, define);

			if (define.tag != ShaderTag_Null) {

				// Process shader define
				if (inBlock) {

					bool end = false;

					switch (define.tag)
					{
					case ShaderTag_End:
						end = true;
						break;
					}

					// End Compilation
					if (end) {

						std::string src;

						const char* beginSrc = inBlock;
						const char* endSrc = line - 1u;

						src.resize(endSrc - beginSrc);
						memcpy(src.data(), beginSrc, src.size());

						if (src.empty() || currentBlockName.empty()) {
							SV_LOG_ERROR("Unvalid block '%s'", currentBlockName.c_str());
						}
						else {

							UserBlock& ub = userBlocks.emplace_back();
							ub.name = std::move(currentBlockName);
							ub.src = std::move(src);

						}
						inBlock = nullptr;
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
						currentBlockName = define.value;
						inBlock = line + lineSize + 1u;
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

		struct SubShaderIntermediate {
			SubShaderID ID;
			std::vector<u8> binData;
		};

		std::vector<SubShaderIntermediate> intermediates;

		// Asign subshaders
		for (SubShaderID i = 0u; i < type->subShaderCount; ++i) {

			SubShaderRegister& reg = type->subShaderRegisters[i];
			
			if (reg.userBlocks.empty()) {
				lib.subShaders[i].shader = reg.defaultShader;
			}
			// Compile new subshader
			else {

				std::string src = reg.src;

				// Replace blocks
				bool found;

				for (SubShaderUserBlock& requestBlock : reg.userBlocks) {

					found = false;

					for (UserBlock block : userBlocks) {

						if (strcmp(requestBlock.name.c_str(), block.name.c_str()) == 0) {
							src.insert(src.begin() + requestBlock.sourcePos, block.src.begin(), block.src.end());
							found = true;
							break;
						}
					}

					if (!found) {
						SV_LOG_ERROR("Subshader block '%s' not found", requestBlock.name.c_str());
					}
				}

				// Compile
				ShaderCompileDesc desc;
				desc.api = graphics_api_get();
				desc.entryPoint = "main";
				desc.majorVersion = 6u;
				desc.minorVersion = 0u;
				desc.shaderType = reg.type;

				std::vector<u8> binData;

				svCheck(graphics_shader_compile_string(&desc, src.c_str(), u32(strlen(src.data())), binData));

				SubShaderIntermediate& inter = intermediates.emplace_back();
				inter.ID = i;
				inter.binData = std::move(binData);
			}

		}

		// Create shaders
		{
			ShaderDesc desc;

			for (SubShaderIntermediate& inter : intermediates) {

				desc.shaderType = type->subShaderRegisters[inter.ID].type;
				desc.binDataSize = inter.binData.size();
				desc.pBinData = inter.binData.data();

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

			archive << u32(intermediates.size());

			for (SubShaderIntermediate& inter : intermediates) {
				archive << inter.ID;
				archive << inter.binData;
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

	Result matsys_shaderlibrarytype_compile(ShaderLibraryType_internal& type, SubShaderID subShaderID, const char* includeName)
	{
		SubShaderRegister& ss = type.subShaderRegisters[subShaderID];
		ss.type = (ShaderType)u32_max;

		// Open file
		std::string filePath = "library/shader_utils/";
		filePath += includeName;
		filePath += ".hlsl";

		FileI file;

		svCheck(file.open(filePath.c_str(), FileOpen_Text));

		file.setPos(0u);

		// Read lines
		std::string line;
		std::stringstream src;
		src << "#include \"core.hlsl\"\n";

		ShaderDefine define;

		while (file.readLine(line)) {

			decomposeDefine(line.c_str(), u32(line.size()), define);

			switch (define.tag)
			{
			case ShaderTag_Name:
				if (ss.name.empty()) ss.name = define.value;
				else SV_LOG_ERROR("Duplicated subshader name");
				break;

			case ShaderTag_UserBlock:
				if (define.value.empty()) SV_LOG_ERROR("Invalid userblock name");
				else {
					SubShaderUserBlock& ub = type.subShaderRegisters[subShaderID].userBlocks.emplace_back();
					ub.name = define.value;
					ub.sourcePos = u32(src.str().size());
				}
				break;

			case ShaderTag_ShaderType:
				if (ss.type == u32_max) {

					if (strcmp(define.value.c_str(), "VertexShader") == 0) {
						ss.type = ShaderType_Vertex;
					}
					else if (strcmp(define.value.c_str(), "PixelShader") == 0) {
						ss.type = ShaderType_Pixel;
					}
					//TODO: More
					else {
						SV_LOG_ERROR("Unknown shader type '%s'", define.value.c_str());
					}

				}
				else {
					SV_LOG_ERROR("Duplicated shader type");
				}
				break;

			default:
				if (!line.empty())
					src << line << '\n';
			}

		}

		file.close();

		// Error checking
		if (ss.name.empty()) {
			SV_LOG_ERROR("The subshader must have a name");
			return Result_CompileError;
		}
		if (ss.type == u32_max) {
			SV_LOG_ERROR("The subshader must have a shader type");
			return Result_CompileError;
		}

		ss.src = src.str();

		return Result_Success;
	}

}