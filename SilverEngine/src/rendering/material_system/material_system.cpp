#include "core.h"

#include "material_system_internal.h"

#include "utils/allocator.h"
#include "utils/io.h"

namespace sv {

	static InstanceAllocator<ShaderLibrary_internal>		g_ShaderLibraries;
	static InstanceAllocator<Material_internal>				g_Materials;
	static InstanceAllocator<CameraBuffer_internal>			g_CameraBuffers;

#define ASSERT_PTR() SV_ASSERT(pInternal != nullptr)

#define PARSE_SHADER_LIBRARY() ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(pInternal)
#define PARSE_MATERIAL() Material_internal& mat = *reinterpret_cast<Material_internal*>(pInternal)
#define PARSE_CAMERA_BUFFER() CameraBuffer_internal& cam = *reinterpret_cast<CameraBuffer_internal*>(pInternal)

	Result matsys_initialize()
	{
		return Result_Success;
	}

	void matsys_update()
	{
		matsys_materials_update();
	}

	Result matsys_close()
	{
		ui32 count;

		count = g_ShaderLibraries.unfreed_count();
		if (count) SV_LOG_WARNING("There are %u unfreed Shader Libraries", count);

		count = g_Materials.unfreed_count();
		if (count) SV_LOG_WARNING("There are %u unfreed Materials", count);

		count = g_CameraBuffers.unfreed_count();
		if (count) SV_LOG_WARNING("There are %u unfreed Camera buffers", count);

		g_ShaderLibraries.clear();
		g_Materials.clear();
		g_CameraBuffers.clear();

		matsys_materials_close();

		return Result_Success;
	}

	// SHADER LIBRARY

	ShaderLibrary::ShaderLibrary(ShaderLibrary&& other) noexcept
	{
		this->operator=(std::move(other));
	}
	ShaderLibrary& ShaderLibrary::operator=(ShaderLibrary&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	Result ShaderLibrary::createFromFile(const char* filePath)
	{
		// TODO: should recreate

		ShaderLibrary_internal lib;

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

		svCheck(matsys_shaderlibrary_compile(lib, src.data()));
		svCheck(matsys_shaderlibrary_construct(lib));

		ShaderLibrary_internal* shaderLibrary = g_ShaderLibraries.create();
		*shaderLibrary = std::move(lib);
		pInternal = shaderLibrary;

		return Result_Success;
	}

	Result ShaderLibrary::createFromString(const char* src)
	{
		// TODO: should recreate
		ShaderLibrary_internal lib;

		svCheck(matsys_shaderlibrary_compile(lib, src));
		svCheck(matsys_shaderlibrary_construct(lib));

		ShaderLibrary_internal* shaderLibrary = g_ShaderLibraries.create();
		*shaderLibrary = std::move(lib);
		pInternal = shaderLibrary;

		return Result_Success;
	}

	Result ShaderLibrary::createFromBinary(const char* name)
	{
		return createFromBinary(hash_string(name));
	}

	Result ShaderLibrary::createFromBinary(size_t hash)
	{
		// TODO: should recreate
		ShaderLibrary_internal lib;

		// Compute hash
		hash_combine(hash, graphics_api_get());

		// Get bin data
		ArchiveI archive;
		svCheck(bin_read(hash, archive));

		archive >> lib.name;

		// Create shaders
		std::vector<ui8> bin;

		lib.vs = nullptr;
		lib.ps = nullptr;
		lib.gs = nullptr;

		ShaderDesc shaderDesc;

		// Vertex shader
		archive >> bin;
		shaderDesc.binDataSize = bin.size();
		shaderDesc.pBinData = bin.data();
		shaderDesc.shaderType = ShaderType_Vertex;
		svCheck(graphics_shader_create(&shaderDesc, &lib.vs));
		bin.clear();

		// Pixel shader
		bool hasShader;
		archive >> hasShader;
		if (hasShader) {
			archive >> bin;
			shaderDesc.binDataSize = bin.size();
			shaderDesc.pBinData = bin.data();
			shaderDesc.shaderType = ShaderType_Pixel;
			svCheck(graphics_shader_create(&shaderDesc, &lib.ps));
			bin.clear();
		}

		// Geometry shader
		archive >> hasShader;
		if (hasShader) {
			archive >> bin;
			shaderDesc.binDataSize = bin.size();
			shaderDesc.pBinData = bin.data();
			shaderDesc.shaderType = ShaderType_Geometry;
			svCheck(graphics_shader_create(&shaderDesc, &lib.gs));
			bin.clear();
		}

		svCheck(matsys_shaderlibrary_construct(lib));

		ShaderLibrary_internal* shaderLibrary = g_ShaderLibraries.create();
		*shaderLibrary = std::move(lib);
		pInternal = shaderLibrary;

		return Result_Success;
	}

	void ShaderLibrary::bind(CameraBuffer* cameraBuffer, CommandList cmd) const
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();

		// Bind shaders
		SV_ASSERT(lib.vs != nullptr);
		graphics_shader_bind(lib.vs, cmd);
		if (lib.ps) graphics_shader_bind(lib.ps, cmd);
		if (lib.gs) graphics_shader_bind(lib.gs, cmd);

		// Bind camera buffer
		if (cameraBuffer) {
			CameraBuffer_internal& cam = **reinterpret_cast<CameraBuffer_internal**>(cameraBuffer);

			for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
				ui32 binding = lib.cameraBufferBinding.i[i];
				if (binding != ui32_max) graphics_constantbuffer_bind(cam.buffer, binding, (ShaderType)i, cmd);
			}
		}
	}

	Result ShaderLibrary::destroy()
	{
		if (pInternal == nullptr) return Result_Success;
		PARSE_SHADER_LIBRARY();

		if (!lib.materials.empty()) {
			SV_LOG_ERROR("Can't destroy a Shader Library with Materials");
			return Result_InvalidUsage;
		}

		// Destroy shaders
		svCheck(graphics_destroy(lib.vs));
		svCheck(graphics_destroy(lib.ps));
		svCheck(graphics_destroy(lib.gs));
		
		g_ShaderLibraries.destroy(&lib);
		pInternal = nullptr;

		return Result_Success;
	}

	const std::vector<ShaderLibraryAttribute>& ShaderLibrary::getAttributes() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.attributes;
	}

	ui32 ShaderLibrary::getTextureCount() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.texturesCount;
	}

	Shader* ShaderLibrary::getShader(ShaderType type) const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.get_shader(type);
	}

	const std::string& ShaderLibrary::getName() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		// TODO: return lib.name
		return "";
	}

	size_t ShaderLibrary::getHashCode() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		// TODO: return lib.hashCode
		return 0u;
	}

	// MATERIAL

	Material::Material(Material&& other) noexcept
	{
	}
	Material& Material::operator=(Material&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	Result Material::create(ShaderLibrary* shaderLibrary, bool dynamic)
	{
		Material_internal mat;

		SV_ASSERT(shaderLibrary);

		mat.dynamic = dynamic;
		mat.shaderLibrary = *reinterpret_cast<ShaderLibrary_internal**>(shaderLibrary);

		if (!mat.shaderLibrary->attributeIndices.empty()) {

			// Reserve rawdata and initialize to 0
			mat.rawMemory = new ui8[mat.shaderLibrary->bufferSizesCount];
			svZeroMemory(mat.rawMemory, mat.shaderLibrary->bufferSizesCount);

			// Asign memory ptrs
			size_t offset = 0u;

			for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
				
				MaterialBuffer& buf = mat.buffers[i];
				size_t size = size_t(mat.shaderLibrary->bufferSizes[i]);

				if (size) {
					buf.rawData = mat.rawMemory + offset;
					offset += size;
				}
			}
			SV_ASSERT(offset == mat.shaderLibrary->bufferSizesCount);
		}

		// Reserve and initialize to 0 texture ptrs
		mat.textures.resize(mat.shaderLibrary->texturesCount, nullptr);

		// Add ptr to library list
		mat.shaderLibrary->materials.push_back(&mat);

		// Add ptr to material update list
		matsys_material_updatelist_add(&mat);

		Material_internal* material = g_Materials.create();
		*material = std::move(mat);
		pInternal = material;

		return Result_Success;
	}

	Result Material::destroy()
	{
		if (pInternal == nullptr) return Result_Success;
		PARSE_MATERIAL();

		// Deallocate rawdata and destroy buffers
		{
			delete[] mat.rawMemory;

			MaterialBuffer* it = mat.buffers;
			MaterialBuffer* end = it + ShaderType_GraphicsCount;

			while (it != end) {
				if (it->rawData) {
					svCheck(graphics_destroy(it->buffer));
				}
				++it;
			}
		}

		// Destroy shaderlibrary reference
		{
			auto& materials = mat.shaderLibrary->materials;
			for (auto it = materials.begin(); it != materials.end(); ++it) {
				if (*it._Ptr == &mat) {
					materials.erase(it);
					break;
				}
			}
		}

		// Remove from update list
		matsys_material_updatelist_rmv(&mat);

		g_Materials.destroy(&mat);
		pInternal = nullptr;

		return Result_Success;
	}

	void Material::bind(CommandList cmd) const
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		// Textures
		for (ui32 shaderType = 0; shaderType < ShaderType_GraphicsCount; ++shaderType) {
			for (ui32 i = 0; i < mat.shaderLibrary->texturesCount; ++i) {
				ui32 bindSlot = mat.shaderLibrary->attributeIndices[i].i[shaderType];
				if (bindSlot != ui32_max)
					graphics_image_bind(mat.textures[i], bindSlot, (ShaderType)shaderType, cmd);
			}
		}

		MaterialBuffer* it = mat.buffers;
		ui32 slot;

		// Vertex Shader
		if (it->buffer) {
			slot = mat.shaderLibrary->bufferBindigs[ShaderType_Vertex];
			if (slot != ui32_max)
				graphics_constantbuffer_bind(it->buffer, slot, ShaderType_Vertex, cmd);
		}
		++it;

		// Pixel Shader
		if (it->buffer) {
			slot = mat.shaderLibrary->bufferBindigs[ShaderType_Pixel];
			if (slot != ui32_max)
				graphics_constantbuffer_bind(it->buffer, slot, ShaderType_Pixel, cmd);
		}
		++it;

		// Geometry Shader
		if (it->buffer) {
			slot = mat.shaderLibrary->bufferBindigs[ShaderType_Geometry];
			if (slot != ui32_max)
				graphics_constantbuffer_bind(it->buffer, slot, ShaderType_Geometry, cmd);
		}

		// TODO: more shaders... ++it;
	}

	void Material::update(CommandList cmd) const
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		matsys_material_update(mat, cmd);
		matsys_material_updatelist_rmv(&mat);
	}

	Result Material::set(const char* name, ShaderAttributeType type, const void* data) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);
		ui32 typeSize = graphics_shader_attribute_size(type);

		const ShaderLibraryAttribute* begin;
		const ShaderLibraryAttribute* it;
		const ShaderLibraryAttribute* endIt;

		if (type == ShaderAttributeType_Unknown) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {

					if (it->type == ShaderAttributeType_Texture) {
						mat.textures[it - begin] = (GPUImage*)data;
					}
					else {
						for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

							ui32 index = mat.shaderLibrary->attributeIndices[it - begin].i[j];
							if (index != ui32_max) {
								MaterialBuffer& mBuf = mat.buffers[j];
								memcpy(mBuf.rawData + index, data, typeSize);

								// Set begin and end indices
								mBuf.updateBegin = std::min(mBuf.updateBegin, index);
								mBuf.updateEnd = std::max(mBuf.updateEnd, index + typeSize);
							}
						}

						matsys_material_updatelist_add(&mat);
					}

					return Result_Success;
				}

				++it;
			}
		}
		else if (type == ShaderAttributeType_Texture) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->texturesCount;

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
					mat.textures[it - begin] = (GPUImage*)data;
					return Result_Success;
				}

				++it;
			}
		}
		else {
			begin = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->texturesCount;
			it = begin;
			endIt = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (it->type == type && string_equal(it->name.c_str(), it->name.size(), name, size)) {

					for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

						ui32 index = mat.shaderLibrary->attributeIndices[it - begin + mat.shaderLibrary->texturesCount].i[j];
						if (index != ui32_max) {
							MaterialBuffer& mBuf = mat.buffers[j];
							memcpy(mBuf.rawData + index, data, typeSize);

							// Set begin and end indices
							mBuf.updateBegin = std::min(mBuf.updateBegin, index);
							mBuf.updateEnd = std::max(mBuf.updateEnd, index + typeSize);
						}
					}

					matsys_material_updatelist_add(&mat);
					return Result_Success;
				}

				++it;
			}
		}

		return Result_NotFound;
	}

	Result Material::get(const char* name, ShaderAttributeType type, void* data) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);
		ui32 typeSize = graphics_shader_attribute_size(type);

		const ShaderLibraryAttribute* begin;
		const ShaderLibraryAttribute* it;
		const ShaderLibraryAttribute* endIt;

		if (type == ShaderAttributeType_Unknown) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {

					if (it->type == ShaderAttributeType_Texture) {
						data = mat.textures[it - begin];
					}
					else {
						for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

							ui32 index = mat.shaderLibrary->attributeIndices[it - begin].i[j];
							if (index != ui32_max) {
								MaterialBuffer& mBuf = mat.buffers[j];
								memcpy(data, mBuf.rawData + index, typeSize);
								matsys_material_updatelist_add(&mat);
								return Result_Success;
							}
						}
					}
				}

				++it;
			}
		}
		else if (type == ShaderAttributeType_Texture) {
			begin = mat.shaderLibrary->attributes.data();
			it = begin;
			endIt = it + mat.shaderLibrary->texturesCount;

			while (it != endIt) {

				if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
					data = mat.textures[it - begin];
					return Result_Success;
				}

				++it;
			}
		}
		else {
			begin = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->texturesCount;
			it = begin;
			endIt = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->attributes.size();

			while (it != endIt) {

				if (it->type == type && string_equal(it->name.c_str(), it->name.size(), name, size)) {

					for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

						ui32 index = mat.shaderLibrary->attributeIndices[it - begin + mat.shaderLibrary->texturesCount].i[j];
						if (index != ui32_max) {
							MaterialBuffer& mBuf = mat.buffers[j];
							memcpy(data, mBuf.rawData + index, typeSize);
							matsys_material_updatelist_add(&mat);
							return Result_Success;
						}
					}
				}

				++it;
			}
		}

		return Result_NotFound;
	}

	ShaderLibrary* Material::getShaderLibrary() const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();
		return reinterpret_cast<ShaderLibrary*>(mat.shaderLibrary);
	}

	// CAMERA BUFFER

	CameraBuffer::CameraBuffer(CameraBuffer&& other) noexcept
	{
		this->operator=(std::move(other));
	}
	CameraBuffer& CameraBuffer::operator=(CameraBuffer&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	Result CameraBuffer::create()
	{
		CameraBuffer_internal cam;

		cam.data.viewMatrix = XMMatrixIdentity();
		cam.data.projectionMatrix = XMMatrixIdentity();
		cam.data.viewProjectionMatrix = XMMatrixIdentity();

		GPUBufferDesc bufferDesc;
		bufferDesc.bufferType = GPUBufferType_Constant;
		bufferDesc.usage = ResourceUsage_Default;
		bufferDesc.CPUAccess = CPUAccess_Write;
		bufferDesc.size = sizeof(CameraBuffer_internal::BufferData);
		bufferDesc.pData = &cam.data;

		svCheck(graphics_buffer_create(&bufferDesc, &cam.buffer));

		CameraBuffer_internal* camBuffer = g_CameraBuffers.create();
		*camBuffer = std::move(cam);
		pInternal = camBuffer;

		return Result_Success;
	}

	Result CameraBuffer::destroy()
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();

		svCheck(graphics_destroy(cam.buffer));
		
		g_CameraBuffers.destroy(&cam);
		pInternal = nullptr;

		return Result_Success;
	}

	void CameraBuffer::setViewMatrix(const XMMATRIX& matrix)
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();
		cam.data.viewMatrix = matrix;
	}
	void CameraBuffer::setProjectionMatrix(const XMMATRIX& matrix)
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();
		cam.data.projectionMatrix = matrix;
	}
	void CameraBuffer::setViewProjectionMatrix(const XMMATRIX& matrix)
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();
		cam.data.viewProjectionMatrix = matrix;
	}
	void CameraBuffer::setPosition(const vec3f& position)
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();
		cam.data.position = position;
	}
	void CameraBuffer::setDirection(const vec3f& direction)
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();
		cam.data.direction = direction;
	}

	void CameraBuffer::update(CommandList cmd)
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();
		graphics_buffer_update(cam.buffer, &cam.data, sizeof(CameraBuffer_internal::BufferData), 0u, cmd);
	}

}