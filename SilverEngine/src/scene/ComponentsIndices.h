#pragma once

#include "..//core.h"
#include "SceneTypes.h"

namespace SV {

	class ComponentsIndices {
		std::vector<std::pair<CompID, size_t>> m_Indices;

	public:
		void AddIndex(CompID ID, size_t index);
		bool GetIndex(CompID ID, size_t& index) const;
		void RemoveIndex(CompID ID);

		inline size_t Size() const noexcept { return m_Indices.size(); }
		inline bool Empty() const noexcept { return m_Indices.empty(); }
		inline CompID operator[](size_t i) const noexcept { return m_Indices[i].first; }
		inline void Clear() noexcept { m_Indices.clear(); }

	};

}