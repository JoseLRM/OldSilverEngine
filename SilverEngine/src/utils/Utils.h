#pragma once

#include "core.h"

namespace _sv {

	bool utils_initialize();
	bool utils_close();

}

namespace sv {

	// String

	std::wstring utils_wstring_parse(const char* c);
	std::string  utils_string_parse(const wchar* c);

	// Timer

	class Time {
		float m_Time;
	public:
		Time() : m_Time(0.f) {}
		Time(float t) : m_Time(t) {}

		inline operator float() const noexcept { return m_Time; }

		inline Time TimeSince(Time time) const noexcept { return Time(time.m_Time - m_Time); }

		inline float GetSecondsFloat() const noexcept { return m_Time; }
		inline ui32 GetSecondsUInt() const noexcept { return ui32(m_Time); }
		inline float GetMillisecondsFloat() const noexcept { return m_Time * 1000.f; }
		inline ui32 GetMillisecondsUInt() const noexcept { return ui32(m_Time * 1000.f); }
	};

	Time timer_now();

	// Hash functions

	template<typename T>
	constexpr void utils_hash_combine(size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	constexpr void utils_hash_string(size_t& seed, const char* str)
	{

	}


}