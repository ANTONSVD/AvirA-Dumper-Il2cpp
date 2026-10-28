#pragma once
#include "Model.hpp"

namespace AvirA
{
	class C_ScriptJson
	{
	public:
		C_ScriptJson(C_Model& model);
		bool Write(const std::string& directory);

	private:
		void CollectUsages();
		void CollectMethods();
		std::string MethodSignature(int typedef_index, int method_index, const std::string& name);

		C_Model& m_model;
		std::string m_methods;
		std::string m_strings;
		std::string m_meta;
		std::string m_meta_methods;
		std::vector<u64> m_addresses;
	};
}
