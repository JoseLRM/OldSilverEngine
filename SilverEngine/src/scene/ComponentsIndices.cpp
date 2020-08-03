#include "core.h"

#include "ComponentsIndices.h"

namespace sv {

	void ComponentsIndices::AddIndex(CompID ID, size_t index)
	{
		m_Indices.push_back(std::make_pair(ID, index));
	}

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