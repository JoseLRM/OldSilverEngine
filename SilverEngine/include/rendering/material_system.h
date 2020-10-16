#pragma once

#include "platform/graphics.h"

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
		size_t getHashCode() const noexcept;

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

		/*
			In both functions, type parameter can be Unknown but specifying the type may perform optimizations
		*/
		Result set(const char* name, ShaderAttributeType type, const void* data) const noexcept;
		Result get(const char* name, ShaderAttributeType type, void* data) const noexcept;

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
		void setDirection(const vec3f& direction);

		void update(CommandList cmd);

	private:
		void* pInternal = nullptr;

	};

}