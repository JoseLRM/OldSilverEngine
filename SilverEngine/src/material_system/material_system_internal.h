#pragma once

#include "material_system.h"

#define svThrow(x) SV_THROW("MATERIAL_SYSTEM_ERROR", x)
#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[MATERIAL_SYSTEM] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[MATERIAL_SYSTEM_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[MATERIAL_SYSTEM_ERROR] "#x, __VA_ARGS__)

namespace sv {

	struct ShaderIndices {
		ui32 i[ShaderType_GraphicsCount];

		ShaderIndices()
		{
			memset(i, ui32_max, sizeof(ui32) * ShaderType_GraphicsCount);
		}
	};

	struct MaterialBuffer {
		GPUBuffer*				buffer = nullptr;
		ui8*					rawData = nullptr;

		ui32 updateBegin = 0u;
		ui32 updateEnd = 0u;
	};

	struct Material_internal;

	struct ShaderLibrary_internal {

		Shader* vs;
		Shader* ps;
		Shader* gs;

		std::vector<ShaderAttribute>	attributes; // Sorted list -> Textures - buffer data. Aligned with attributeIndices
		std::vector<ShaderIndices>		attributeIndices; // Contains information about attr. offset (or texture slot) and if a shader contains or not this attr.
		ui32							bufferSizes[ShaderType_GraphicsCount] = {};
		ui32							texturesCount;

		std::vector<Material_internal*> materials;

		inline Shader* get_shader(ui32 type) const noexcept 
		{
			switch (type)
			{
			case sv::ShaderType_Vertex:
				return vs;
			case sv::ShaderType_Pixel:
				return ps;
			case sv::ShaderType_Geometry:
				return gs;
			default:
				return nullptr;
			}
		}

	};

	struct Material_internal {

		ShaderLibrary_internal* shaderLibrary;
		MaterialBuffer			buffers[ShaderType_GraphicsCount] = {};
		std::vector<GPUImage*>	textures;
		bool					dynamic;
		bool					inUpdateList = false;

	};

	Result	matsys_initialize();
	void	matsys_update();
	Result	matsys_close();

	void matsys_material_update(Material_internal& mat, CommandList cmd);

}