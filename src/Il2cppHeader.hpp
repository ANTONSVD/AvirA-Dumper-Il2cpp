#pragma once
#include <unordered_set>
#include "Model.hpp"

namespace AvirA
{
	class C_Il2cppHeader
	{
	public:
		C_Il2cppHeader(C_Model& model);
		bool Write(const std::string& path);

	private:
		void EmitValueType(int typedef_index, std::string& out);
		void EmitClass(int typedef_index, std::string& out);
		void EmitMembers(const S_Layout& layout, bool is_class, std::string& out) const;

		C_Model& m_model;
		std::unordered_set<int> m_done;
		std::unordered_set<int> m_busy;
	};
}
