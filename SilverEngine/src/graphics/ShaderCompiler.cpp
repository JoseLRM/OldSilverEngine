#include "core.h"

#include "ShaderCompiler.h"

namespace SV {
	namespace Graphics {
		bool GetShaderBinPath(const char* filePath, SV_GFX_API api, std::string& binPath)
		{
			SV::TxtFile file;
			if (!file.OpenR((SV_SRC_PATH + std::string(filePath)).c_str())) {
				SV::LogE("Shader '%s' not found", filePath);
				return false;
			}
			
			std::string line;
			while (file.ReadLine(line)) {

				bool found = false;

				switch (api)
				{
				case SV_GFX_API_DX11:
					if (line[0] == 'd' && line[1] == 'x') {
						binPath = line.c_str() + 3;
						found = true;
					}
					break;
				case SV_GFX_API_VULKAN:
					if (line[0] == 'v' && line[1] == 'k') {
						binPath = line.c_str() + 3;
						found = true;
					}
					break;
				}

				if (found) break;
			}

			binPath = SV_SRC_PATH + binPath;

			file.Close();
			return true;
		}
	}
}