#pragma once

#include "platform/graphics.h"
#include "simulation/asset_system.h"

namespace sv {

	struct ShaderLibraryAttribute {
		std::string			name;
		ShaderAttributeType type;
	};

	class CameraBuffer;

	class ShaderLibrary {

	public:
		ShaderLibrary() = default;

		ShaderLibrary(const ShaderLibrary& other) = delete;
		ShaderLibrary(ShaderLibrary&& other) noexcept;
		ShaderLibrary& operator=(const ShaderLibrary& other) = delete;
		ShaderLibrary& operator=(ShaderLibrary&& other) noexcept;

		Result createFromFile(const char* filePath);
		Result createFromString(const char* src);
		Result createFromBinary(const char* name);
		Result createFromBinary(size_t hashCode);

		void bind(CameraBuffer* cameraBuffer, CommandList cmd) const;

		Result destroy();

		// getters

		const std::vector<ShaderLibraryAttribute>& getAttributes() const noexcept;
		ui32 getTextureCount() const noexcept;
		Shader* getShader(ShaderType type) const noexcept;
		const std::string& getName() const noexcept;
		size_t getNameHashCode() const noexcept;

		const std::string& getType() const noexcept;
		bool isType(const char* type) const noexcept;

	private:
		void* pInternal = nullptr;

	};

	class Material {

	public:
		Material() = default;

		Material(const Material& other) = delete;
		Material(Material&& other) noexcept;
		Material& operator=(const Material& other) = delete;
		Material& operator=(Material&& other) noexcept;

		Result create(ShaderLibrary* shaderLibrary, bool dynamic = false);
		Result destroy();

		void bind(CommandList cmd) const;
		void update(CommandList cmd) const;

		// Setters

		Result setInt32(const char* name, i32 n) const noexcept;
		Result setInt64(const char* name, i64 n) const noexcept;
		Result setUInt32(const char* name, ui32 n) const noexcept;
		Result setUInt64(const char* name, ui64 n) const noexcept;
		// TODO: Result setHalf(const char* name, ? n) const noexcept;
		Result setDouble(const char* name, double n) const noexcept;
		Result setFloat(const char* name, float n) const noexcept;
		Result setFloat2(const char* name, const vec2f& n) const noexcept;
		Result setFloat3(const char* name, const vec3f& n) const noexcept;
		Result setFloat4(const char* name, const vec4f& n) const noexcept;
		Result setBool(const char* name, BOOL n) const noexcept;
		Result setChar(const char* name, char n) const noexcept;
		Result setMat3(const char* name, const XMFLOAT3X3& n) const noexcept;
		Result setMat4(const char* name, const XMMATRIX& n) const noexcept;
		
		Result setRaw(const char* name, ShaderAttributeType type, const void* data) const noexcept;

		Result setGPUImage(const char* name, GPUImage* image) const noexcept;
		Result setTexture(const char* name, const TextureAsset& texture) const noexcept;

		inline Result setMat4(const char* name, const XMFLOAT4X4& n) const noexcept { return setMat4(name, XMLoadFloat4x4(&n)); }
		inline Result setFloat4(const char* name, const Color4f& n) const noexcept { return setFloat4(name, *reinterpret_cast<const vec4f*>(&n)); }
		inline Result setFloat3(const char* name, const Color3f& n) const noexcept { return setFloat3(name, *reinterpret_cast<const vec3f*>(&n)); }
		inline Result setFloat4(const char* name, const Color& n) const noexcept { return setFloat4(name, vec4f{ float(n.r) / 255.f, float(n.g) / 255.f, float(n.b) / 255.f, float(n.a) / 255.f }); }

		// Getters

		Result getInt32(const char* name, i32& n) const noexcept;
		Result getInt64(const char* name, i64& n) const noexcept;
		Result getUInt32(const char* name, ui32& n) const noexcept;
		Result getUInt64(const char* name, ui64& n) const noexcept;
		// Result getHalf(const char* name, ? n) const noexcept;
		Result getDouble(const char* name, double& n) const noexcept;
		Result getFloat(const char* name, float& n) const noexcept;
		Result getFloat2(const char* name, vec2f& n) const noexcept;
		Result getFloat3(const char* name, vec3f& n) const noexcept;
		Result getFloat4(const char* name, vec4f& n) const noexcept;
		Result getBool(const char* name, BOOL& n) const noexcept;
		Result getChar(const char* name, char& n) const noexcept;
		Result getMat3(const char* name, XMFLOAT3X3& n) const noexcept;
		Result getMat4(const char* name, XMMATRIX& n) const noexcept;

		Result getRaw(const char* name, ShaderAttributeType type, void* data) const noexcept;

		Result getGPUImage(const char* name, GPUImage*& image) const noexcept;
		Result getTexture(const char* name, TextureAsset& texture) const noexcept;
		Result getImage(const char* name, GPUImage*& image) const noexcept;

		inline Result getMat4(const char* name, XMFLOAT4X4& n) const noexcept { XMMATRIX m; Result res = getMat4(name, m); XMStoreFloat4x4(&n, m); return res; }
		inline Result getFloat4(const char* name, Color4f& n) const noexcept { return getFloat4(name, *reinterpret_cast<vec4f*>(&n)); }
		inline Result getFloat3(const char* name, Color3f& n) const noexcept { return getFloat3(name, *reinterpret_cast<vec3f*>(&n)); }
		inline Result getFloat4(const char* name, Color& n) const noexcept { vec4f vec; Result res = getFloat4(name, vec); n = { ui8(vec.x * 255.f), ui8(vec.y * 255.f), ui8(vec.z * 255.f), ui8(vec.w * 255.f) }; return res; }

		ShaderLibrary* getShaderLibrary() const noexcept;

	private:
		void* pInternal = nullptr;

	};

	class CameraBuffer {

	public:
		CameraBuffer() = default;

		CameraBuffer(const CameraBuffer& other) = delete;
		CameraBuffer(CameraBuffer&& other) noexcept;
		CameraBuffer& operator=(const CameraBuffer& other) = delete;
		CameraBuffer& operator=(CameraBuffer&& other) noexcept;

		Result create();
		Result destroy();

		void setViewMatrix(const XMMATRIX& matrix);
		void setProjectionMatrix(const XMMATRIX& matrix);
		void setViewProjectionMatrix(const XMMATRIX& matrix);
		void setPosition(const vec3f& position);
		void setDirection(const vec4f& direction);

		void update(CommandList cmd);

	private:
		void* pInternal = nullptr;

	};

	// Assets

	class ShaderLibraryAsset : public Asset {
	public:
		inline ShaderLibrary* get() const noexcept { return reinterpret_cast<ShaderLibrary*>(m_Ref.get()); }
		inline ShaderLibrary* operator->() const noexcept { return get(); }
	};

	class MaterialAsset : public Asset {
	public:
		inline Material* get() const noexcept { return reinterpret_cast<Material*>(m_Ref.get()); }
		inline Material* operator->() const noexcept { return get(); }

		Result createFile(const char* filePath, ShaderLibraryAsset& shaderLibraryAsset);
		Result serialize();
	};

}