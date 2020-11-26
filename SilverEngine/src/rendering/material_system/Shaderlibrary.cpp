#include "core.h"

#include "material_system_internal.h"
#include "utils/io.h"

namespace sv {

	Result matsys_shaderlibrary_allocate(ShaderLibrary_internal& lib, ShaderLibrary** pInternal);
	Result matsys_shaderlibrary_construct(ShaderLibrary_internal& lib);

	Result matsys_shaderlibrary_create_from_file(const char* filePath, ShaderLibrary** pShaderLibrary)
	{
#ifdef SV_RES_PATH
		std::string filePathStr = SV_RES_PATH;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return Result_NotFound;

		std::vector<char> src;
		src.resize(file.tellg());
		file.seekg(0u);
		file.read(src.data(), src.size());

		file.close();

		return matsys_shaderlibrary_create_from_string(src.data(), pShaderLibrary);
	}

	Result matsys_shaderlibrary_create_from_string(const char* src, ShaderLibrary** pShaderLibrary)
	{
		ShaderLibrary_internal lib;

		svCheck(matsys_shaderlibrary_compile(lib, src));
		svCheck(matsys_shaderlibrary_construct(lib));

		return matsys_shaderlibrary_allocate(lib, pShaderLibrary);
	}

	Result matsys_shaderlibrary_create_from_binary(const char* name, ShaderLibrary** pShaderLibrary)
	{
		return matsys_shaderlibrary_create_from_binary(hash_string(name), pShaderLibrary);
	}

	Result matsys_shaderlibrary_create_from_binary(size_t hash, ShaderLibrary** pShaderLibrary)
	{
		// TODO: should recreate
		ShaderLibrary_internal lib;

		// Compute hash
		hash_combine(hash, graphics_api_get());

		// Get bin data
		ArchiveI archive;
		svCheck(bin_read(hash, archive));

		archive >> lib.name;

		// Find type
		{
			std::string type;
			archive >> type;

			ShaderLibraryType_internal* rend = typeFind(type.c_str());
			if (rend == nullptr) {
				SV_LOG_ERROR("ShaderLibrary Type '%s' not found", type.c_str());
				return Result_NotFound;
			}

			lib.type = rend;
		}

		// Create default subshaders
		ui32 customSubShaderRequest = 0u;
		{
			size_t subShaderCount = lib.type->subShaderCount;

			for (size_t i = 0u; i < subShaderCount; ++i) {

				SubShader& ss = lib.subShaders[i];
				ss.shader = lib.type->subShaderRegisters[i].defaultShader;

				if (ss.shader == nullptr)
					++customSubShaderRequest;
			}
		}

		// Create custom subshaders
		{
			ui32 count;
			archive >> count;

			if (count != customSubShaderRequest) {
				SV_LOG_ERROR("The shaderlibrary must have %u subshaders, it currently have %u", customSubShaderRequest, count);
				return Result_InvalidFormat;
			}

			while (count--) {

				SubShaderID ID;
				std::vector<ui8> data;
				archive >> ID >> data;

				ShaderDesc desc;
				desc.shaderType = lib.type->subShaderRegisters[ID].type;
				desc.binDataSize = data.size();
				desc.pBinData = data.data();

				svCheck(graphics_shader_create(&desc, &lib.subShaders[ID].shader));
			}
		}

		svCheck(matsys_shaderlibrary_construct(lib));

		return matsys_shaderlibrary_allocate(lib, pShaderLibrary);
	}

	Result matsys_shaderlibrary_destroy(ShaderLibrary* pInternal)
	{
		if (pInternal == nullptr) return Result_Success;
		PARSE_SHADER_LIBRARY();

		if (lib.matCount.load() != 0) {
			SV_LOG_ERROR("Can't destroy a Shader Library with Materials");
			return Result_InvalidUsage;
		}

		// Destroy shaders
		for (SubShader s : lib.subShaders) {
			// TODO: Check if it is a default shader
			svCheck(graphics_destroy(s.shader));
		}

		g_ShaderLibraries.destroy(lib);
		return Result_Success;
	}

	void matsys_shaderlibrary_bind_subshader(ShaderLibrary* pInternal, SubShaderID subShaderID, CommandList cmd)
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		SV_ASSERT(lib.type->subShaderCount > subShaderID);

		SubShader& shader = lib.subShaders[subShaderID];

		// Bind Camera Buffer
		if (shader.cameraSlot != ui32_max && lib.cameraBuffer) {
			graphics_constantbuffer_bind(lib.cameraBuffer, shader.cameraSlot, graphics_shader_type(shader.shader), cmd);
		}

		// Bind Shader
		graphics_shader_bind(shader.shader, cmd);
	}

	void matsys_shaderlibrary_bind_camerabuffer(ShaderLibrary* pInternal, CameraBuffer& cameraBuffer, CommandList cmd)
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();

		lib.cameraBuffer = cameraBuffer.gpuBuffer;
	}

	const std::vector<MaterialAttribute>& matsys_shaderlibrary_attributes(ShaderLibrary* pInternal)
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.matInfo.attributes;
	}

	const std::vector<std::string>& matsys_shaderlibrary_textures(ShaderLibrary* pInternal)
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.matInfo.textures;
	}

	// SHADER LIBRARY CREATION

	Result matsys_shaderlibrary_construct(ShaderLibrary_internal& lib)
	{
		ui32 subShaderCount = lib.type->subShaderCount;
		
		// Initialization
		for (ui32 i = 0u; i < MAX_SUBSHADERS; ++i)
			lib.matInfo.bufferSizes[i] = 0u;

		for (ui32 i = 0u; i < MAX_SUBSHADERS; ++i)
			lib.matInfo.bufferBindings[i] = ui32_max;

		lib.matInfo.bufferSizesCount = 0u;

		// Construction
		for (ui32 i = 0u; i < subShaderCount; ++i) {

			SubShader& ss = lib.subShaders[i];
			const ShaderInfo& sInfo = *graphics_shader_info_get(ss.shader);

			// Constant Buffers
			for (const ShaderInfo::ResourceBuffer& buffer : sInfo.constantBuffers) {

				// Material
				if (strcmp(buffer.name.c_str(), "__Material__") == 0) {

					for (const ShaderAttribute& att : buffer.attributes) {

						ui32 attIndex = ui32_max;

						// Find if the attribute exist
						for (ui32 j = 0u; j < lib.matInfo.attributes.size(); ++j) {

							MaterialAttribute& matAtt = lib.matInfo.attributes[j];

							if (matAtt.type == att.type && strcmp(matAtt.name.c_str(), att.name.c_str()) == 0) {
								attIndex = j;
								break;
							}
						}

						// If not found, create new attribute
						if (attIndex == ui32_max) {

							attIndex = ui32(lib.matInfo.attributes.size());

							MaterialAttribute& matAtt = lib.matInfo.attributes.emplace_back();
							matAtt.name = att.name;
							matAtt.type = att.type;

							lib.matInfo.attributeOffsets.emplace_back().init(ui32_max);
						}

						// Save offset
						SubShaderIndices& o = lib.matInfo.attributeOffsets[attIndex];
						SV_ASSERT(o.i[i] == ui32_max);

						o.i[i] = att.offset;
					}

					// Set buffer size and slot data
					lib.matInfo.bufferSizes[i] = buffer.size;
					lib.matInfo.bufferBindings[i] = buffer.bindingSlot;
					lib.matInfo.bufferSizesCount += buffer.size;
				}

				// Camera
				else if (strcmp(buffer.name.c_str(), "__Camera__") == 0) {
					SV_ASSERT(ss.cameraSlot == ui32_max);
					ss.cameraSlot = buffer.bindingSlot;
				}
			}

			// Textures
			for (const ShaderInfo::ResourceImage& image : sInfo.images) {

				if (image.name[0] == '_') continue;
				ui32 imgSlot = ui32_max;

				// Find if texture exist
				for (ui32 j = 0u; j < lib.matInfo.textures.size(); ++j) {

					const std::string& texStr = lib.matInfo.textures[j];

					if (strcmp(texStr.c_str(), image.name.c_str()) == 0) {
						imgSlot = j;
						break;
					}
				}

				// If not found, create new image
				if (imgSlot == ui32_max) {

					imgSlot = ui32(lib.matInfo.textures.size());
					lib.matInfo.textures.emplace_back() = image.name; 
					lib.matInfo.textureSlots.emplace_back().init(ui32_max);
				}

				SubShaderIndices& slots = lib.matInfo.textureSlots[imgSlot];
				slots.i[i] = image.bindingSlot;
			}
		}

		return Result_Success;
	}

	Result matsys_shaderlibrary_allocate(ShaderLibrary_internal& lib, ShaderLibrary** pInternal)
	{
		// Recreate
		if (*pInternal) {
			ShaderLibrary_internal& old = *reinterpret_cast<ShaderLibrary_internal*>(*pInternal);

			// Recreate materials
			lib.matCount = old.matCount.load();

			for (auto& pool : g_Materials) {
				for (Material_internal& mat : pool) {

					if (mat.shaderLibrary == &old) {
						svCheck(mat.recreate(lib));
						mat.shaderLibrary = &old;
					}
				}
			}
			

			// Move data
			old = std::move(lib);
		}
		// Allocate
		else {
			ShaderLibrary_internal* shaderLibrary = &g_ShaderLibraries.create();
			*shaderLibrary = std::move(lib);
			*pInternal = reinterpret_cast<ShaderLibrary*>(shaderLibrary);
		}

		return Result_Success;
	}

	ShaderLibrary_internal& ShaderLibrary_internal::operator=(const ShaderLibrary_internal& other)
	{
		cameraBuffer = other.cameraBuffer;
		matCount = other.matCount.load();
		matInfo = other.matInfo;
		name = other.name;
		nameHashCode = other.nameHashCode;
		for(ui32 i = 0u; i < MAX_SUBSHADERS; ++i) 
			subShaders[i] = other.subShaders[i];
		type = other.type;
		return *this;
	}

	ShaderLibrary_internal& ShaderLibrary_internal::operator=(ShaderLibrary_internal&& other) noexcept
	{
		cameraBuffer = other.cameraBuffer;
		matCount = other.matCount.load();
		matInfo = std::move(other.matInfo);
		name = std::move(other.name);
		nameHashCode = other.nameHashCode;
		for (ui32 i = 0u; i < MAX_SUBSHADERS; ++i)
			subShaders[i] = std::move(other.subShaders[i]);
		type = other.type;
		return *this;
	}

}