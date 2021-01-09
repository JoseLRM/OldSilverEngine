#include "core.h"

#include "core/utils/helper.h"
#include "core/utils/io.h"

#ifdef SV_RES_PATH
#define PARSE_RES_PATH() std::string filePathStr; \
	if (!sv::path_is_absolute(filePath)) { \
		filePathStr = SV_RES_PATH; \
		filePathStr += filePath; \
		filePath = filePathStr.c_str(); \
	} 
#else
#define PARSE_RES_PATH()
#endif

namespace sv {

	Result file_read_binary(const char* filePath, u8** pData, size_t* pSize)
	{
		PARSE_RES_PATH();

		std::ifstream stream;
		stream.open(filePath, std::ios::binary | std::ios::ate);
		if (!stream.is_open())
			return Result_NotFound;

		*pSize = stream.tellg();
		*pData = new u8[*pSize];

		stream.seekg(0u);

		stream.read((char*)*pData, *pSize);

		stream.close();

		return Result_Success;
	}

	Result file_read_binary(const char* filePath, std::vector<u8>& data)
	{
		PARSE_RES_PATH();

		std::ifstream stream;
		stream.open(filePath, std::ios::binary | std::ios::ate);
		if (!stream.is_open())
			return Result_NotFound;

		data.resize(stream.tellg());

		stream.seekg(0u);

		stream.read((char*)data.data(), data.size());

		stream.close();

		return Result_Success;
	}

	Result file_write_binary(const char* filePath, const u8* data, size_t size, bool append)
	{
		PARSE_RES_PATH();

		std::ofstream stream;
		stream.open(filePath, std::ios::binary | (append ? std::ios::app : 0u));
		if (!stream.is_open())
			return Result_NotFound;

		stream.write((const char*)data, size);

		stream.close();

		return Result_Success;
	}

	Result file_write_text(const char* filePath, const char* str, size_t size, bool append)
	{
		PARSE_RES_PATH();

		std::ofstream stream;
		stream.open(filePath, (append ? std::ios::app : 0u));
		if (!stream.is_open())
			return Result_NotFound;

		stream.write(str, size);

		stream.close();

		return Result_Success;
	}

	Result file_remove(const char* filePath)
	{
		PARSE_RES_PATH();
		return (remove(filePath) != 0) ? Result_NotFound : Result_Success;
	}

	Result FileO::open(const char* filePath, FileOpenFlags flags)
	{
		close();

		PARSE_RES_PATH();

		u32 f = 0u;

		if (flags & FileOpen_Append) {

			f |= std::ios::app;
		}
	
		if (!(flags & FileOpen_Text)) f |= std::ios::binary;

		stream.open(filePath, f);

		if (stream.is_open()) {
			return Result_Success;
		}
		else return Result_NotFound;
	}

	void FileO::close()
	{
		stream.close();
	}

	bool FileO::isOpen()
	{
		return stream.is_open();
	}

	void FileO::write(const u8* data, size_t size)
	{
		stream.write((const char*)data, size);
	}

	void FileO::writeLine(const char* str)
	{
		stream.write(str, strlen(str));
	}

	void FileO::writeLine(const std::string& str)
	{
		stream.write(str.data(), str.size());
	}

	Result FileI::open(const char* filePath, FileOpenFlags flags)
	{
		close();

		PARSE_RES_PATH();

		u32 f = 0u;

		if (!(flags & FileOpen_Text)) f |= std::ios::binary;

		stream.open(filePath, f);

		if (stream.is_open()) {
			return Result_Success;
		}
		else return Result_NotFound;
	}

	void FileI::close()
	{
		stream.close();
	}

	bool FileI::isOpen()
	{
		return stream.is_open();
	}

	void FileI::setPos(size_t pos)
	{
		stream.seekg(pos);
	}

	size_t FileI::getPos()
	{
		return stream.tellg();
	}

	size_t FileI::getSize()
	{
		size_t pos = stream.tellg();
		stream.seekg(stream.end);
		size_t size = stream.tellg();
		stream.seekg(pos);
		return size;
	}

	void FileI::read(u8* data, size_t size)
	{
		stream.read((char*)data, size);
	}

	bool FileI::readLine(char* buf, size_t bufSize)
	{
		return (bool)stream.getline((char*)buf, bufSize);
	}

	bool FileI::readLine(std::string& str)
	{
		return (bool)std::getline(stream, str);
	}

}