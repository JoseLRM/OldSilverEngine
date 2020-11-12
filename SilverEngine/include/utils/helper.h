#pragma once

#include "core.h"

namespace sv {

	std::wstring parse_wstring(const char* c);
	std::string  parse_string(const wchar* c);

	bool path_is_absolute(const char* path);
	void path_clear(char* path);

	bool path_is_absolute(const wchar* path);

	// Hash functions

	template<typename T>
	constexpr void hash_combine(size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	size_t hash_string(const char* str);

}