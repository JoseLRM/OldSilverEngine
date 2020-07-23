#pragma once

#include "core.h"

namespace SV {

	class TxtFile {
		size_t m_Size;
		bool m_ReadMode;

		std::ifstream m_Input;
		std::ofstream m_Output;

	public:
		TxtFile();
		~TxtFile();

		bool OpenR(const char* filePath);
		bool OpenW(const char* filePath);
		void Close();

		bool ReadLine(std::string& str);

		void WriteLine(const char* line, size_t size);

		// Setters

		void SetPos(size_t pos);

		// Getters

		inline size_t GetSize() const noexcept { return m_Size; }
		size_t GetPos();
		bool InReadMode() const noexcept;
		bool InWriteMode() const noexcept;

	};

}