#include "core.h"

#include "BinFile.h"

namespace sv {

	BinFile::BinFile() : m_Size(0)
	{
	}

	BinFile::~BinFile()
	{
		Close();
	}

	bool BinFile::OpenR(const char* filePath)
	{
		m_ReadMode = true;
		Close();
		m_Input = std::ifstream();

#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		m_Input.open(filePathStr.c_str(), std::ios::ate | std::ios::binary);
#else
		m_Input.open(filePath, std::ios::ate | std::ios::binary);
#endif

		if (!m_Input.is_open()) return false;

		m_Size = m_Input.tellg();
		m_Input.seekg(0);

		return true;
	}

	bool BinFile::OpenW(const char* filePath)
	{
		m_ReadMode = false;
		Close();
		m_Output = std::ofstream();

#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		m_Output.open(filePathStr.c_str(), std::ios::ate | std::ios::binary);
#else
		m_Output.open(filePath, std::ios::ate | std::ios::binary);
#endif

		if (!m_Output.is_open()) return false;

		m_Size = m_Output.tellp();
		m_Output.seekp(0);

		return true;
	}

	void BinFile::Close()
	{
		if (m_Input.is_open()) m_Input.close();
		if (m_Output.is_open()) m_Output.close();
	}

	void BinFile::Read(void* dst, size_t size)
	{
		m_Input.read((char*)dst, size);
	}

	void BinFile::Write(const void* src, size_t size)
	{
		m_Output.write((const char*)src, size);
	}

	void BinFile::SetPos(size_t pos)
	{
		if (m_ReadMode) {
			m_Input.seekg(pos);
		}
		else {
			m_Output.seekp(pos);
		}
	}

	size_t BinFile::GetPos()
	{
		return m_ReadMode ? m_Input.tellg() : m_Output.tellp();
	}

	bool BinFile::InReadMode() const noexcept
	{
		return m_ReadMode && m_Input.is_open();
	}
	bool BinFile::InWriteMode() const noexcept
	{
		return !m_ReadMode && m_Output.is_open();
	}

}