#pragma once

#include "core.h"

namespace sv {

	class ArchiveO {

		size_t m_Size;
		size_t m_Capacity;
		u8* m_Data;

	public:
		ArchiveO();
		~ArchiveO();

		void reserve(size_t size);
		void write(const void* data, size_t size);
		void erase(size_t size);
		void clear();

		Result save_file(const char* filePath, bool append = false);

		template<typename T>
		inline ArchiveO& operator<<(const T& t)
		{
			write(&t, sizeof(T));
			return *this;
		}

		template<typename T>
		inline ArchiveO& operator<<(const std::vector<T>& vec)
		{
			size_t size = vec.size();
			write(&size, sizeof(size_t));

			for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
				this->operator<<(*it);
			}
			return *this;
		}

		inline ArchiveO& operator<<(const char* txt)
		{
			size_t txtLength = strlen(txt);
			write(&txtLength, sizeof(size_t));
			write(txt, txtLength);
			return *this;
		}

		inline ArchiveO& operator<<(const wchar* txt)
		{
			size_t txtLength = wcslen(txt);
			write(&txtLength, sizeof(size_t));
			write(txt, txtLength * sizeof(wchar));
			return *this;
		}

		inline ArchiveO& operator<<(const std::string& str)
		{
			size_t txtLength = str.size();
			write(&txtLength, sizeof(size_t));
			write(str.data(), txtLength);
			return *this;
		}

		inline ArchiveO& operator<<(const std::wstring& str)
		{
			size_t txtLength = str.size();
			write(&txtLength, sizeof(size_t));
			write(str.data(), txtLength * sizeof(wchar));
			return *this;
		}

	private:
		void allocate(size_t size);
		void free();

	};

	class ArchiveI {
		u8* m_Data;
		size_t m_Size;
		size_t m_Pos;

	public:
		ArchiveI();
		~ArchiveI();

		Result open_file(const char* filePath);

		void read(void* data, size_t size);

		inline size_t pos_get() const noexcept { return m_Pos; }
		inline void pos_set(size_t pos) noexcept { m_Pos = pos; }
		inline size_t size() const noexcept { return m_Size; }

		void clear();

		template<typename T>
		ArchiveI& operator>>(T& t)
		{
			read(&t, sizeof(T));
			return *this;
		}

		template<typename T>
		ArchiveI& operator>>(std::vector<T>& vec)
		{
			size_t size;
			read(&size, sizeof(size_t));
			size_t lastSize = vec.size();
			vec.resize(lastSize + size);
			read(vec.data() + lastSize, sizeof(T) * size);
			return *this;
		}

		ArchiveI& operator>>(std::string& str)
		{
			size_t txtLength;
			read(&txtLength, sizeof(size_t));
			str.resize(txtLength);
			read(str.data(), txtLength);
			return *this;
		}

		ArchiveI& operator>>(std::wstring& str)
		{
			size_t txtLength;
			read(&txtLength, sizeof(size_t));
			str.resize(txtLength);
			read(str.data(), txtLength * sizeof(wchar));
			return *this;
		}

	};

	Result load_image(const char* filePath, void** pdata, u32* width, u32* height);

	Result bin_read(size_t hash, std::vector<u8>& data);
	Result bin_read(size_t hash, ArchiveI& archive);

	Result bin_write(size_t hash, const void* data, size_t size);
	Result bin_write(size_t hash, ArchiveO& archive);

}