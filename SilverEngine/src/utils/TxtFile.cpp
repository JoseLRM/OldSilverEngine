#include "core.h"

#include "TxtFile.h"

namespace sv {

	TxtFile::TxtFile() : m_Size(0)
	{
	}

	TxtFile::~TxtFile()
	{
		Close();
	}

	bool TxtFile::OpenR(const char* filePath)
	{
		m_ReadMode = true;
		Close();
		m_Input = std::ifstream();
		m_Input.open(filePath, std::ios::ate);

		if (!m_Input.is_open()) return false;

		m_Size = m_Input.tellg();
		m_Input.seekg(0);

		return true;
	}

	bool TxtFile::OpenW(const char* filePath)
	{
		m_ReadMode = false;
		Close();
		m_Output = std::ofstream();
		m_Output.open(filePath, std::ios::ate);

		if (!m_Output.is_open()) return false;

		m_Size = m_Output.tellp();
		m_Output.seekp(0);

		return true;
	}

	void TxtFile::Close()
	{
		if (m_Input.is_open()) m_Input.close();
		if (m_Output.is_open()) m_Output.close();
	}

	bool TxtFile::ReadLine(std::string& str)
	{
		std::getline(m_Input, str);
		return str.size();
	}

	void TxtFile::WriteLine(const char* line, size_t size)
	{
		m_Output.write(line, size);
	}

	void TxtFile::SetPos(size_t pos)
	{
		if (m_ReadMode) {
			m_Input.seekg(pos);
		}
		else {
			m_Output.seekp(pos);
		}
	}

	size_t TxtFile::GetPos()
	{
		return m_ReadMode ? m_Input.tellg() : m_Output.tellp();
	}

	bool TxtFile::InReadMode() const noexcept
	{
		return m_ReadMode && m_Input.is_open();
	}
	bool TxtFile::InWriteMode() const noexcept
	{
		return !m_ReadMode && m_Output.is_open();
	}

}