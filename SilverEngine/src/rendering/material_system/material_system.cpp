#include "core.h"

#include "material_system_internal.h"

#include "utils/allocator.h"
#include "utils/io.h"

namespace sv {

	static InstanceAllocator<ShaderLibrary_internal, 200u>	g_ShaderLibraries;
	static InstanceAllocator<Material_internal, 200u>		g_Materials;
	static InstanceAllocator<CameraBuffer_internal, 200u>	g_CameraBuffers;

#define ASSERT_PTR() SV_ASSERT(pInternal != nullptr)

#define PARSE_SHADER_LIBRARY() ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(pInternal)
#define PARSE_MATERIAL() Material_internal& mat = *reinterpret_cast<Material_internal*>(pInternal)
#define PARSE_CAMERA_BUFFER() CameraBuffer_internal& cam = *reinterpret_cast<CameraBuffer_internal*>(pInternal)


	Result matsys_initialize()
	{
		svCheck(matsys_asset_register());
		return Result_Success;
	}

	void matsys_update()
	{
		matsys_materials_update();
	}

	Result matsys_close()
	{
		ui32 count;

		count = g_Materials.unfreed_count();
		if (count) SV_LOG_WARNING("There are %u unfreed Materials", count);

		count = g_ShaderLibraries.unfreed_count();
		if (count) {
			SV_LOG_WARNING("There are %u unfreed Shader Libraries", count);

			for (auto& pool : g_ShaderLibraries) {
				for (ShaderLibrary_internal& lib : pool) {
					SV_LOG_ERROR("TODO");
				}
			}
		}

		count = g_CameraBuffers.unfreed_count();
		if (count) SV_LOG_WARNING("There are %u unfreed Camera buffers", count);

		g_ShaderLibraries.clear();
		g_Materials.clear();
		g_CameraBuffers.clear();

		matsys_materials_close();

		return Result_Success;
	}

	// SHADER LIBRARY

	Result matsys_shaderlibrary_allocate(ShaderLibrary_internal& lib, void*& pInternal)
	{
		// Recreate
		if (pInternal) {
			ShaderLibrary_internal& old = *reinterpret_cast<ShaderLibrary_internal*>(pInternal);

			// Recreate materials
			lib.materials = std::move(old.materials);

			for (Material_internal* mat : lib.materials) {

				svCheck(matsys_material_recreate(*mat, lib));
				mat->shaderLibrary = &old;

			}

			// Move data
			old = std::move(lib);
		}
		// Allocate
		else {
			ShaderLibrary_internal* shaderLibrary = &g_ShaderLibraries.create();
			*shaderLibrary = std::move(lib);
			pInternal = shaderLibrary;
		}

		return Result_Success;
	}

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

		return matsys_shaderlibrary_allocate(lib, pInternal);
	}

	Result ShaderLibrary::createFromString(const char* src)
	{
		// TODO: should recreate
		ShaderLibrary_internal lib;

		svCheck(matsys_shaderlibrary_compile(lib, src));
		svCheck(matsys_shaderlibrary_construct(lib));

		return matsys_shaderlibrary_allocate(lib, pInternal);
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
		archive >> lib.type;

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

		return matsys_shaderlibrary_allocate(lib, pInternal);
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
		
		g_ShaderLibraries.destroy(lib);
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
		return lib.name;
	}

	size_t ShaderLibrary::getNameHashCode() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.nameHashCode;
	}

	const std::string& ShaderLibrary::getType() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.type;
	}

	bool ShaderLibrary::isType(const char* type) const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return string_equal(lib.type.c_str(), lib.type.size(), type, strlen(type));
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
		SV_ASSERT(shaderLibrary);

		Material_internal mat;
		svCheck(matsys_material_create(mat, *reinterpret_cast<ShaderLibrary_internal**>(shaderLibrary), dynamic, false));

		Material_internal* material = &g_Materials.create();
		*material = std::move(mat);
		pInternal = material;

		// Add ptr to library list
		material->shaderLibrary->materials.push_back(material);

		// Add ptr to material update list
		matsys_material_updatelist_add(material);

		return Result_Success;
	}

	Result Material::destroy()
	{
		if (pInternal == nullptr) return Result_Success;
		PARSE_MATERIAL();

		Result res = matsys_material_destroy(mat);

		g_Materials.destroy(mat);
		pInternal = nullptr;

		return res;
	}

	void Material::bind(CommandList cmd) const
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		// Textures
		for (ui32 shaderType = 0; shaderType < ShaderType_GraphicsCount; ++shaderType) {
			for (ui32 i = 0; i < mat.shaderLibrary->texturesCount; ++i) {
				ui32 bindSlot = mat.shaderLibrary->attributeIndices[i].i[shaderType];
				if (bindSlot != ui32_max) {

					MaterialTexture& tex = mat.textures[i];

					if (tex.texture.hasReference()) {
						graphics_image_bind(tex.texture.get(), bindSlot, (ShaderType)shaderType, cmd);
					}
					else graphics_image_bind(tex.image, bindSlot, (ShaderType)shaderType, cmd);
					
				}
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

	Result Material::setInt32(const char* name, i32 n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Int32, &n);
	}
	Result Material::setInt64(const char* name, i64 n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Int64, &n);
	}
	Result Material::setUInt32(const char* name, ui32 n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_UInt32, &n);
	}
	Result Material::setUInt64(const char* name, ui64 n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_UInt64, &n);
	}
	Result Material::setDouble(const char* name, double n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Double, &n);
	}
	Result Material::setFloat(const char* name, float n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Float, &n);
	}
	Result Material::setFloat2(const char* name, const vec2f& n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Float2, &n);
	}
	Result Material::setFloat3(const char* name, const vec3f& n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Float3, &n);
	}
	Result Material::setFloat4(const char* name, const vec4f& n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Float4, &n);
	}
	Result Material::setBool(const char* name, BOOL n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Boolean, &n);
	}
	Result Material::setChar(const char* name, char n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Char, &n);
	}
	Result Material::setMat3(const char* name, const XMFLOAT3X3& n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Mat3, &n);
	}
	Result Material::setMat4(const char* name, const XMMATRIX& n) const noexcept
	{
		return setRaw(name, ShaderAttributeType_Mat4, &n);
	}
	Result Material::setRaw(const char* name, ShaderAttributeType type, const void* data) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);

		const ShaderLibraryAttribute* begin = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->texturesCount;
		const ShaderLibraryAttribute* it = begin;
		const ShaderLibraryAttribute* endIt = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->attributes.size();

		while (it != endIt) {

			if (it->type == type && string_equal(it->name.c_str(), it->name.size(), name, size)) {

				for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

					ui32 index = mat.shaderLibrary->attributeIndices[it - begin + mat.shaderLibrary->texturesCount].i[j];
					if (index != ui32_max) {

						ui32 typeSize = graphics_shader_attribute_size(type);

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

		return Result_NotFound;
	}

	Result Material::setGPUImage(const char* name, GPUImage* image) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);

		const ShaderLibraryAttribute* begin = mat.shaderLibrary->attributes.data();
		const ShaderLibraryAttribute* it = begin;
		const ShaderLibraryAttribute* endIt = it + mat.shaderLibrary->texturesCount;

		while (it != endIt) {

			if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
				MaterialTexture& tex = mat.textures[it - begin];
				tex.texture.unload();
				tex.image = image;
				return Result_Success;
			}

			++it;
		}

		return Result_NotFound;
	}
	Result Material::setTexture(const char* name, const TextureAsset& texture) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);

		const ShaderLibraryAttribute* begin = mat.shaderLibrary->attributes.data();
		const ShaderLibraryAttribute* it = begin;
		const ShaderLibraryAttribute* endIt = it + mat.shaderLibrary->texturesCount;

		while (it != endIt) {

			if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
				MaterialTexture& tex = mat.textures[it - begin];
				tex.texture = texture;
				tex.image = nullptr;
				return Result_Success;
			}

			++it;
		}

		return Result_NotFound;
	}

	Result Material::getInt32(const char* name, i32& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Int32, &n);
	}
	Result Material::getInt64(const char* name, i64& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Int64, &n);
	}
	Result Material::getUInt32(const char* name, ui32& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_UInt32, &n);
	}
	Result Material::getUInt64(const char* name, ui64& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_UInt64, &n);
	}
	Result Material::getDouble(const char* name, double& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Double, &n);
	}
	Result Material::getFloat(const char* name, float& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Float, &n);
	}
	Result Material::getFloat2(const char* name, vec2f& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Float2, &n);
	}
	Result Material::getFloat3(const char* name, vec3f& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Float3, &n);
	}
	Result Material::getFloat4(const char* name, vec4f& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Float4, &n);
	}
	Result Material::getBool(const char* name, BOOL& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Boolean, &n);
	}
	Result Material::getChar(const char* name, char& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Char, &n);
	}
	Result Material::getMat3(const char* name, XMFLOAT3X3& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Mat3, &n);
	}
	Result Material::getMat4(const char* name, XMMATRIX& n) const noexcept
	{
		return getRaw(name, ShaderAttributeType_Mat4, &n);
	}

	Result Material::getRaw(const char* name, ShaderAttributeType type, void* data) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);

		const ShaderLibraryAttribute* begin = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->texturesCount;
		const ShaderLibraryAttribute* it = begin;
		const ShaderLibraryAttribute* endIt = mat.shaderLibrary->attributes.data() + mat.shaderLibrary->attributes.size();

		while (it != endIt) {

			if (it->type == type && string_equal(it->name.c_str(), it->name.size(), name, size)) {

				for (ui32 j = 0; j < ShaderType_GraphicsCount; ++j) {

					ui32 index = mat.shaderLibrary->attributeIndices[it - begin + mat.shaderLibrary->texturesCount].i[j];
					if (index != ui32_max) {
						MaterialBuffer& mBuf = mat.buffers[j];
						memcpy(data, mBuf.rawData + index, graphics_shader_attribute_size(type));
						return Result_Success;
					}
				}
			}

			++it;
		}

		return Result_NotFound;
	}

	Result Material::getGPUImage(const char* name, GPUImage*& image) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);

		const ShaderLibraryAttribute* begin = mat.shaderLibrary->attributes.data();
		const ShaderLibraryAttribute* it = begin;
		const ShaderLibraryAttribute* endIt = it + mat.shaderLibrary->texturesCount;

		while (it != endIt) {

			if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
				image = mat.textures[it - begin].image;
				return Result_Success;
			}

			++it;
		}

		return Result_NotFound;
	}

	Result Material::getTexture(const char* name, TextureAsset& texture) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);

		const ShaderLibraryAttribute* begin = mat.shaderLibrary->attributes.data();
		const ShaderLibraryAttribute* it = begin;
		const ShaderLibraryAttribute* endIt = it + mat.shaderLibrary->texturesCount;

		while (it != endIt) {

			if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
				texture = mat.textures[it - begin].texture;
				return Result_Success;
			}

			++it;
		}

		return Result_NotFound;
	}

	Result Material::getImage(const char* name, GPUImage*& image) const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();

		size_t size = strlen(name);

		const ShaderLibraryAttribute* begin = mat.shaderLibrary->attributes.data();
		const ShaderLibraryAttribute* it = begin;
		const ShaderLibraryAttribute* endIt = it + mat.shaderLibrary->texturesCount;

		while (it != endIt) {

			if (string_equal(it->name.c_str(), it->name.size(), name, size)) {
				MaterialTexture& tex = mat.textures[it - begin];
				if (tex.texture.hasReference()) image = tex.texture.get();
				else image = tex.image;
				return Result_Success;
			}

			++it;
		}

		return Result_NotFound;
	}

	ShaderLibrary* Material::getShaderLibrary() const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();
		return reinterpret_cast<ShaderLibrary*>(&mat.shaderLibrary);
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

		CameraBuffer_internal* camBuffer = &g_CameraBuffers.create();
		*camBuffer = std::move(cam);
		pInternal = camBuffer;

		return Result_Success;
	}

	Result CameraBuffer::destroy()
	{
		ASSERT_PTR();
		PARSE_CAMERA_BUFFER();

		svCheck(graphics_destroy(cam.buffer));
		
		g_CameraBuffers.destroy(cam);
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
	void CameraBuffer::setDirection(const vec4f& direction)
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