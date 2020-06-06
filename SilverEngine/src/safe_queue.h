#pragma once

#include "core.h"

namespace SV {

	template<typename T, size_t SIZE>
	class safe_queue {
		T m_Data[SIZE];

		size_t m_Head = 0u;
		size_t m_Tail = 0u;

		std::mutex m_Mutex;

	public:
		safe_queue() : m_Mutex() {}

		bool push(const T& obj)
		{
			bool result = false;
			m_Mutex.lock();

			size_t nextHead = (m_Head + 1) % SIZE;

			if (nextHead != m_Tail) {
				m_Data[m_Head] = obj;
				m_Head = nextHead;
				result = true;
			}

			m_Mutex.unlock();
			return result;
		}
		bool pop(T& obj)
		{
			bool result = false;
			m_Mutex.lock();

			if (m_Tail != m_Head) {
				obj = m_Data[m_Tail];
				m_Tail = (m_Tail + 1) % SIZE;
				result = true;
			}

			m_Mutex.unlock();
			return result;
		}
		size_t size()
		{
			return (m_Head >= m_Tail) ? (m_Head - m_Tail) : (SIZE - (m_Tail - m_Head));
		}

	};

}