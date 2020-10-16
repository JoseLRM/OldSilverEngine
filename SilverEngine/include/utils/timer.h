#pragma once

#include "core.h"

namespace sv {

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

	struct Date {
		ui32 year;
		ui32 month;
		ui32 day;
		ui32 hour;
		ui32 minute;
		ui32 second;
	};

	Time timer_now();
	Date timer_date();

}