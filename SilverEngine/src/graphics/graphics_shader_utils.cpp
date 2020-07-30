#include "core.h"

#include "graphics_shader_utils.h"

namespace sv {
	bool graphics_shader_binpath(const char* filePath, SV_GFX_API api, std::string& binPath)
	{
		sv::TxtFile file;
		if (!file.OpenR(filePath)) {
			sv::log_error("Shader '%s' not found", filePath);
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

#ifdef SV_SRC_PATH
		binPath = SV_SRC_PATH + binPath;
#endif

		file.Close();
		return true;
	}
}