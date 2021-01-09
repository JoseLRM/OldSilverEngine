#include "core.h"

#include "core/utils/helper.h"
#include "core/utils/io.h"

#include "core/platform/impl.h"

namespace sv {

	//std::string path_resolve_str(const char* path)
	//{
	//	const char* respath = io_respath_get();
	//
	//	size_t respathSize = respath ? strlen(respath) : 0u;
	//	size_t pathSize = strlen(path);
	//
	//	std::string res;
	//	res.resize(respathSize + pathSize);
	//
	//	memcpy(res.data(), respath, respathSize);
	//	memcpy(res.data() + respathSize, path, pathSize);
	//
	//	return res;
	//}
	//
	//u32 path_resolve(const char* path, char* buff, size_t bufSize)
	//{
	//	const char* respath = io_respath_get();
	//	size_t respathSize = respath ? strlen(respath) : 0u;
	//	size_t writeSize = std::min(bufSize, respathSize);
	//	// TODO
	//	SV_LOG_ERROR("TODO->path_resolve");
	//	return u32_max;
	//	memcpy(buff, respath, writeSize);
	//	bufSize -= writeSize;
	//
	//	if (bufSize) {
	//
	//		size_t pathSize = strlen(path);
	//		writeSize = std::min(bufSize, respathSize);
	//		memcpy(buff + respathSize, path, writeSize);
	//		bufSize -= writeSize;
	//
	//		if (bufSize) {
	//			buff[pathSize + respathSize] = '\0';
	//			return 0u;
	//		}
	//		else return (respathSize + pathSize) + (writeSize + respathSize) + 1u;
	//	}
	//	else return respathSize - writeSize + strlen(path) + 1u;
	//}

	std::wstring parse_wstring(const char* c)
	{
		std::wstring str;
		str.resize(strlen(c));
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, c, int(str.size()), &str[0], int(str.size()));
		return str;
	}

	std::string parse_string(const wchar* c)
	{
		size_t size = wcslen(c) + 1u;
		std::string str;
		str.resize(size);
		size_t i;
		wcstombs_s(&i, str.data(), size, c, size);
		return str;
	}

	bool path_is_absolute(const char* filePath)
	{
		return filePath[0] != '\0' && filePath[1] == ':';
	}
	void path_clear(char* path)
	{
		while (*path != '\0') {
			
			switch (*path)
			{
			case '//':
				*path = '/';
				break;

			case '\\':
				*path = '/';
				break;
			}

			++path;
		}
	}

	bool path_is_absolute(const wchar* filePath)
	{
		return filePath[0] != L'\0' && filePath[1] == L':';
	}

	size_t hash_string(const char* str)
	{
		// TEMPORAL
		size_t res = 0u;
		size_t strLength = strlen(str);
		hash_combine(res, strLength);
		while (*str != '\0') {
			hash_combine(res, (const size_t)* str);
			++str;
		}
		return res;
	}

}