#include "core.h"

#include "utils/io.h"

namespace sv {

	// OUTPUT

	ArchiveO::ArchiveO() : m_Capacity(0u), m_Size(0u), m_Data(nullptr)
	{}

	ArchiveO::~ArchiveO()
	{
		free();
	}

	void ArchiveO::reserve(size_t size)
	{
		if (m_Size + size > m_Capacity) {

			size_t newSize = size_t(double(m_Size + size) * 1.5);
			allocate(newSize);

		}
	}

	void ArchiveO::write(const void* data, size_t size)
	{
		reserve(size);

		memcpy(m_Data + m_Size, data, size);
		m_Size += size;
	}

	void ArchiveO::erase(size_t size)
	{
		SV_ASSERT(size <= m_Size);
		m_Size -= size;
	}

	void ArchiveO::clear()
	{
		m_Size = 0u;
	}

	Result ArchiveO::save_file(const char* filePath, bool append)
	{
#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

		std::ofstream stream;

		int mode = std::ios::binary;
		if (append) mode |= std::ios::app | std::ios::ate;

		stream.open(filePath, mode);

		if (!stream.is_open()) return Result_NotFound;

		stream.write((const char*)m_Data, m_Size);

		stream.close();
		return Result_Success;
	}

	void ArchiveO::allocate(size_t size)
	{
		ui8* newData = (ui8*)operator new(size);
		if (m_Data) {
			memcpy(newData, m_Data, m_Size);
			operator delete[](m_Data);
		}
		m_Capacity = size;
		m_Data = newData;
	}

	void ArchiveO::free()
	{
		if (m_Data) {
			operator delete[](m_Data);
			m_Data = nullptr;
			m_Size = 0u;
			m_Capacity = 0u;
		}
	}

	// INPUT

	ArchiveI::ArchiveI() : m_Size(0u), m_Pos(0u), m_Data(nullptr)
	{
	}

	ArchiveI::~ArchiveI()
	{
		clear();
	}

	Result ArchiveI::open_file(const char* filePath)
	{
		std::ifstream stream;

#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

		stream.open(filePath, std::ios::ate | std::ios::binary);

		if (!stream.is_open()) return Result_NotFound;

		m_Size = stream.tellg();
		stream.seekg(0u);
		m_Pos = 0u;

		m_Data = new ui8[m_Size];
		stream.read((char*)m_Data, m_Size);

		stream.close();

		return Result_Success;
	}

	void ArchiveI::read(void* data, size_t size)
	{
		SV_ASSERT(m_Pos + size <= m_Size && m_Data != nullptr);
		memcpy(data, m_Data + m_Pos, size);
		m_Pos += size;
	}

	void ArchiveI::clear()
	{
		if (m_Data) {
			operator delete[](m_Data);
			m_Data = nullptr;
			m_Size = 0u;
			m_Pos = 0u;
		}
	}

}