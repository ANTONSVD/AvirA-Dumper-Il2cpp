#pragma once
#include "Attributes.hpp"

namespace AvirA
{
	class C_DumpWriter
	{
	public:
		C_DumpWriter(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary);
		bool Write(const std::string& path);

	private:
		void WriteTypedef(int image_index, int typedef_index, std::string& body);
		std::string MethodModifiers(u32 flags) const;
		std::string TypeAccess(u32 visibility) const;
		std::string MethodAccess(u32 access) const;
		std::string FieldAccess(u32 access) const;
		std::string MethodGenericParams(int container_idx) const;

		const C_Il2cppMetadata& m_metadata;
		const C_Il2cppBinary& m_binary;
		C_Attributes m_attributes;
	};
}
