#include "debug/editor.h"
#include "utils/allocators.h"
#include "utils/string.h"
#include "core/engine.h"

namespace sv {

	SV_AUX void buffer_write(RawList& raw_list, const char* str)
	{
		raw_list.write_back(str, string_size(str));
	}

	SV_AUX void parse_path(char* str)
	{
		size_t size = string_size(str);
		foreach(i, size)
			if (str[i] == '/') str[i] = '\\';
	}

	bool compile_plugin(const CompilePluginDesc& desc)
	{
		RawList buffer;

		// TODO
		const char* name = "Game";

		//buffer_write(buffer, "CALL system\\shell.bat\n");
		
		buffer_write(buffer, "CALL cl ");

		// Compiler flags
		buffer_write(buffer, "-MT -nologo -EHa -GR -Oi -WX -W4 -FC /std:c++14 -Z7 ");

		// Disable warnings
		buffer_write(buffer, "-wd4201 -wd4127 -wd4211 -wd4238 -wd4459 -wd4996 -wd4456 -wd4281 -wd4100 -wd4530 ");

		// Defines		
		buffer_write(buffer, "-DSV_PLATFORM_WIN=1 -DSV_SLOW=0 -DSV_EDITOR=1 -DSV_GFX=0 ");

		// Include Paths
		buffer_write(buffer, "/I include\\ ");

		// Src path
		{
			char srcpath[FILEPATH_SIZE + 1u];
			string_copy(srcpath, desc.plugin_path, FILEPATH_SIZE + 1u);
			string_append(srcpath, "src/", FILEPATH_SIZE + 1u);
			
			parse_path(srcpath);

			string_append(srcpath, "build_unit.cpp ", FILEPATH_SIZE + 1u);
			buffer_write(buffer, srcpath);
		}

		// LINKER

		buffer_write(buffer, "/link ");

		// Linker flags
		
		buffer_write(buffer, "/DLL /incremental:no /LIBPATH:\"system\\bin\\\" SilverEngine.lib ");

		// Dst
		{
			char dst[FILEPATH_SIZE + 1u];
			string_copy(dst, desc.plugin_path, FILEPATH_SIZE + 1u);
			string_append(dst, name, FILEPATH_SIZE + 1u);
			string_append(dst, ".dll", FILEPATH_SIZE + 1u);
			
			parse_path(dst);

			buffer_write(buffer, "/out:");
			buffer_write(buffer, dst);
			buffer_write(buffer, " ");
		}

		// Output
		{
			char dst[FILEPATH_SIZE + 1u];
			string_copy(dst, desc.plugin_path, FILEPATH_SIZE + 1u);
			string_append(dst, "int/", FILEPATH_SIZE + 1u);
			
			parse_path(dst);

			buffer_write(buffer, "/PDB:");
			buffer_write(buffer, dst);
			buffer_write(buffer, name);
			buffer_write(buffer, ".pdb ");
			
			buffer_write(buffer, " > ");
			buffer_write(buffer, dst);
			buffer_write(buffer, "build_output.txt");
		}

		system((const char*)buffer.data());
						
		const char* respath = "int/build_output.txt";
		
		// Get compilation result
		char* str;
		size_t size;
		if (file_read_text(respath, &str, &size)) {

			SV_LOG_INFO("Compilation result:\n");
			SV_LOG("%s\n", str);
			SV_FREE_MEMORY(str);

			if (!file_remove(respath)) {
				SV_LOG_ERROR("Can't delete the compilation result file");
			}
		}
		else SV_LOG_ERROR("Can't find the compilation result");

		return true;
	}
	
}
