#pragma once

#include "core.h"

namespace sv {

	class Time {
		float m_Time;
	public:
		constexpr Time() : m_Time(0.f) {}
		constexpr Time(float t) : m_Time(t) {}

		inline operator float() const noexcept { return m_Time; }

		inline Time TimeSince(Time time) const noexcept { return Time(time.m_Time - m_Time); }

		inline float GetSecondsFloat() const noexcept { return m_Time; }
		inline u32 GetSecondsUInt() const noexcept { return u32(m_Time); }
		inline float GetMillisecondsFloat() const noexcept { return m_Time * 1000.f; }
		inline u32 GetMillisecondsUInt() const noexcept { return u32(m_Time * 1000.f); }
	};

	struct Date {
		u32 year;
		u32 month;
		u32 day;
		u32 hour;
		u32 minute;
		u32 second;
	};

	Time timer_now();
	Date timer_date();

}