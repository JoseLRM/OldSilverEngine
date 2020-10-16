#include "core.h"

#include "utils/io.h"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stbi_lib.h"

namespace sv {

	Result load_image(const char* filePath, void** pdata, ui32* width, ui32* height)
	{
		int w = 0, h = 0, bits = 0;

#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		void* data = stbi_load(filePathStr.c_str(), &w, &h, &bits, 4);
#else
		void* data = stbi_load(filePath, &w, &h, &bits, 4);
#endif

		* pdata = nullptr;
		*width = w;
		*height = h;

		if (!data) return Result_NotFound;
		*pdata = data;
		return Result_Success;
	}

	std::string bin_filepath(size_t hash)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
#ifdef SV_SRC_PATH
		filePath = SV_SRC_PATH + filePath;
#endif
		return filePath;
	}

	Result bin_read(size_t hash, std::vector<ui8>& data)
	{
		std::string filePath = bin_filepath(hash);

		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		if (!file.is_open()) return Result_NotFound;

		size_t size = file.tellg();
		file.seekg(0u);
		data.resize(size);
		file.read((char*)data.data(), size);

		file.close();
		return Result_Success;
	}

	Result bin_read(size_t hash, ArchiveI& archive)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		svCheck(archive.open_file(filePath.c_str()));
		return Result_Success;
	}

	Result bin_write(size_t hash, const void* data, size_t size)
	{
		std::string filePath = bin_filepath(hash);

		std::ofstream file(filePath, std::ios::binary);
		if (!file.is_open()) return Result_NotFound;

		file.write((const char*)data, size);
		file.close();

		return Result_Success;
	}

	Result bin_write(size_t hash, ArchiveO& archive)
	{
		std::string filePath = "bin/" + std::to_string(hash) + ".bin";
		svCheck(archive.save_file(filePath.c_str()));
		return Result_Success;
	}

}