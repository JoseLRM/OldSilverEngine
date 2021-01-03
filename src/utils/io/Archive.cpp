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
		return file_write_binary(filePath, m_Data, m_Size, append);
	}

	void ArchiveO::allocate(size_t size)
	{
		u8* newData = (u8*)operator new(size);
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
		return file_read_binary(filePath, &m_Data, &m_Size);
	}

	void ArchiveI::read(void* data, size_t size)
	{
		if (m_Pos + size > m_Size) {
			size_t invalidSize = (m_Pos + size) - m_Size;
			SV_ZERO_MEMORY((u8*)data + size - invalidSize, invalidSize);
			size -= invalidSize;
			SV_LOG_WARNING("Archive reading, out of bounds");
		}
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