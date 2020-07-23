#pragma once

#include "core.h"

namespace SV {

	class BinFile {
		size_t m_Size;
		bool m_ReadMode;

		std::ifstream m_Input;
		std::ofstream m_Output;

	public:
		BinFile();
		~BinFile();

		bool OpenR(const char* filePath);
		bool OpenW(const char* filePath);
		void Close();

		void Read(void* dst, size_t size);
		void Write(const void* src, size_t size);

		// Setters

		void SetPos(size_t pos);

		// Getters

		inline size_t GetSize() const noexcept { return m_Size; }
		size_t GetPos();
		bool InReadMode() const noexcept;
		bool InWriteMode() const noexcept;

	};

}