#include "core.h"

#include "ComponentsIndices.h"

namespace SV {

	void ComponentsIndices::AddIndex(CompID ID, size_t index)
	{
		if (m_Indices.empty()) {
			m_Indices.push_back(std::make_pair(ID, index));
			return;
		}

		for (ui32 i = 0; i < m_Indices.size(); ++i) {
			if (m_Indices[i].first <= ID) {
				m_Indices.insert(m_Indices.begin() + i + 1u, std::make_pair(ID, index));
				return;
			}
		}
	}

#define BINARY_SEARCH 1

	bool ComponentsIndices::GetIndex(CompID ID, size_t& index) const
	{
		for (auto it = m_Indices.begin(); it != m_Indices.end(); ++it) {
			if (it->first == ID) {
				index = it->second;
				return true;
			}
		}
		
		return false;
	}

	void ComponentsIndices::RemoveIndex(CompID ID)
	{
		for (auto it = m_Indices.begin(); it != m_Indices.end(); ++it) {
			if (it->first == ID) {
				m_Indices.erase(it);
				return;
			}
		}
	}

}