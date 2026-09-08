#pragma once
#include "Il2cppMetadata.hpp"
#include "Il2cppBinary.hpp"

namespace AvirA
{
	class C_DummyDll
	{
	public:
		C_DummyDll(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary);
		bool Generate(const std::string& directory);

	private:
		const C_Il2cppMetadata& m_metadata;
		const C_Il2cppBinary& m_binary;
	};
}
