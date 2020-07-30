#pragma once

#include "..//core.h"

namespace sv {

	template<typename T>
	class RenderList {
		T* m_Data;
		ui32 m_Size;
		ui32 m_Pos;

		ui32 m_LastPos;
		double m_DynamicSize;

	public:
		RenderList() : m_Data(nullptr), m_Size(0u), m_Pos(0u), m_LastPos(0u), m_DynamicSize(0.0)
		{}
		~RenderList()
		{
			Free();
			m_Pos = 0u;
		}

		RenderList(const RenderList& other)
		{
			this->operator=(other);
		}
		RenderList(RenderList&& other) noexcept
		{
			this->operator=(std::move(other));
		}

		RenderList<T>& operator=(const RenderList<T>& other)
		{
			Free();
			Alloc(other.m_Pos);
			m_Pos = other.m_Pos;
			memcpy(m_Data, other.m_Data, m_Pos * sizeof(T));
			return *this;
		}
		RenderList<T>& operator=(RenderList<T>&& other) noexcept
		{
			m_Data = other.m_Data;
			m_Size = other.m_Size;
			m_Pos = other.m_Pos;
			other.m_Data = nullptr;
			other.m_Size = 0u;
			other.m_Pos = 0u;
			return *this;
		}

		template<class... Args>
		void Emplace(Args&&... args) noexcept
		{
			ReserveRequest();
			new(m_Data + m_Pos) T(std::forward<Args>(args)...);
			m_Pos++;
		}
		void Push(const T& t) noexcept
		{
			ReserveRequest();
			new(m_Data + m_Pos) T(t);
			m_Pos++;
		}
		void Push(T&& t) noexcept
		{
			ReserveRequest();
			new(m_Data + m_Pos) T(std::move(t));
			m_Pos++;
		}

		void Reserve(ui32 count) noexcept
		{
			if (count + m_Pos > m_Size)
			{
				Alloc(m_Pos + count);
				m_DynamicSize = double(m_Size);
			}
		}
		void Pop() noexcept
		{
			SV_ASSERT(m_Pos != 0u);
			m_Pos--;
		}

		T& operator[](ui32 i) noexcept {
			SV_ASSERT(i < m_Pos);
			return m_Data[i];
		}
		const T& operator[](ui32 i) const noexcept {
			SV_ASSERT(i < m_Pos);
			return m_Data[i];
		}

		T& back() noexcept
		{
			SV_ASSERT(m_Pos != 0u);
			return m_Data[m_Pos - 1u];
		}
		const T& back() const noexcept
		{
			SV_ASSERT(m_Pos != 0u);
			return m_Data[m_Pos - 1u];
		}

		ui32 Size() const noexcept
		{
			return m_Pos;
		}
		ui32 Capacity() const noexcept
		{
			return m_Size;
		}
		bool Empty() const noexcept
		{
			return m_Pos == 0u;
		}
		void Reset() noexcept
		{
			double pos = m_Pos;
			double size = m_Size;

			m_DynamicSize = m_DynamicSize * 0.8 + pos * 0.2;

			if (m_Pos > m_LastPos) {
				Reserve(m_Pos - m_LastPos);
			}
			else if (m_DynamicSize / size < 0.8) {
				Alloc(ui32(m_DynamicSize));
			}

			m_LastPos = m_Pos;
			m_Pos = 0u;
		}

	private:
		void ReserveRequest()
		{
			if (m_Size <= m_Pos) {
				Reserve(5u);
			}
		}
		void Alloc(ui32 size) noexcept
		{
			T* newData = new T[size];
			memcpy(newData, m_Data, m_Pos * sizeof(T));

			Free();
			m_Size = size;
			m_Data = newData;
		}
		void Free() noexcept
		{
			if (m_Data) {
				delete[] m_Data;
				m_Data = nullptr;
				m_Size = 0u;
			}
		}

	};

}