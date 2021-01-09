#include "core.h"

#include "core/utils/timer.h"

namespace sv {

	static std::chrono::steady_clock::time_point g_InitialTime = std::chrono::steady_clock::now();

	static std::chrono::steady_clock::time_point timer_now_chrono()
	{
		return std::chrono::steady_clock::now();
	}

	Time timer_now()
	{
		return Time(std::chrono::duration<float>(timer_now_chrono() - g_InitialTime).count());
	}

	Date timer_date()
	{
		__time64_t t;
		_time64(&t);
		tm time;
		_localtime64_s(&time, &t);

		Date date;
		date.year = time.tm_year + 1900;
		date.month = time.tm_mon + 1;
		date.day = time.tm_mday;
		date.hour = time.tm_hour;
		date.minute = time.tm_min;
		date.second = time.tm_sec;

		return date;
	}

}