#pragma once

#include "core.h"

#include "utils/Archive.h"
#include "utils/sv_math.h"
#include "utils/Version.h"
#include "utils/safe_queue.h"

namespace sv {

	// String

	std::wstring utils_wstring_parse(const char* c);
	std::string  utils_string_parse(const wchar* c);

	// Loader

	Result utils_loader_image(const char* filePath, void** pdata, ui32* width, ui32* height);

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
	std::chrono::steady_clock::time_point timer_start_point();


	// Hash functions

	template<typename T>
	constexpr void utils_hash_combine(size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	size_t utils_hash_string(const char* str);

	// References
	// TODO: I am using standard ptrs

	template<typename T>
	class SharedRef {
		std::shared_ptr<T> m_Ref;

	public:
		SharedRef() = default;
		SharedRef(std::shared_ptr<T>&& shared) { m_Ref = std::move(shared); }
		SharedRef(const SharedRef& other) 
		{
			m_Ref = other.m_Ref;
		}
		SharedRef(SharedRef&& other) noexcept
		{
			m_Ref = std::move(other.m_Ref);
		}

		SharedRef<T>& operator=(const SharedRef& other)
		{
			m_Ref = other.m_Ref;
			return *this;
		}
		SharedRef<T>& operator=(SharedRef&& other) noexcept
		{
			m_Ref = std::move(other.m_Ref);
			return *this;
		}

		T& operator*() { return *m_Ref.get(); }
		const T& operator*() const { return *m_Ref.get(); }
		T* operator->() { return m_Ref.get(); }
		const T* operator->() const { return m_Ref.get(); }
		T* Get() { return m_Ref.get(); }
		const T* Get() const { return m_Ref.get(); }
		ui32 RefCount() const { return m_Ref.use_count(); }

		void Delete() { m_Ref.reset(); }

	};

	template<typename T>
	class WeakRef {
		std::weak_ptr<T> m_Ref;

	public:
		WeakRef() = default;
		WeakRef(std::weak_ptr<T>&& other) { m_Ref = std::move(other); }
		WeakRef(const WeakRef& other)
		{
			m_Ref = other.m_Ref;
		}
		WeakRef(const SharedRef<T>& shared)
		{
			m_Ref = *reinterpret_cast<const std::shared_ptr<T>*>(&shared);
		}
		WeakRef(WeakRef&& other) noexcept
		{
			m_Ref = std::move(other.m_Ref);
		}

		WeakRef<T>& operator=(const WeakRef& other)
		{
			m_Ref = other.m_Ref;
			return *this;
		}
		WeakRef<T>& operator=(WeakRef&& other) noexcept
		{
			m_Ref = std::move(other.m_Ref);
			return *this;
		}

		bool Expired() const
		{
			return m_Ref.expired();
		}

		SharedRef<T> GetShared() const noexcept
		{
			return { m_Ref.lock() };
		}

	};

	template<typename T, typename... Args>
	SharedRef<T> create_shared(Args&&... args)
	{
		return { std::make_shared<T>(std::forward<Args>(args)...) };
	}

}

