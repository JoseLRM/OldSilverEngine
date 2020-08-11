#include "core.h"

#include "graphics_internal.h"

namespace sv {
	bool graphics_shader_binpath(const char* filePath, GraphicsAPI api, std::string& binPath)
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
			case GraphicsAPI_Vulkan:
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