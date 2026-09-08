#pragma once
#include "Il2cppBinary.hpp"

namespace AvirA
{
	struct S_BlobValue
	{
		u32 kind = 0;
		bool is_null = false;
		u64 uval = 0;
		i64 ival = 0;
		double fval = 0.0;
		std::string sval;
		std::vector<S_BlobValue> items;
	};

	class C_Attributes
	{
	public:
		C_Attributes(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary);

		std::string CustomAttributes(int image_index, u32 token, const std::string& indent) const;
		bool DefaultValueText(int type_idx, int data_idx, std::string& out_text, u64& out_fail_ptr) const;

		static std::string FormatFloat(float value);
		static std::string FormatDouble(double value);
		static std::string EscapeDumpString(const std::string& text);

	private:
		bool ReadConstant(u32 kind, u64& pos, u64 end, S_BlobValue& out_value) const;
		u32 ReadEncodedType(u64& pos, u64 end, bool& ok) const;
		std::string FormatBlobValue(const S_BlobValue& value) const;
		static std::string StripAttributeWord(std::string name);

		const C_Il2cppMetadata& m_metadata;
		const C_Il2cppBinary& m_binary;
	};
}
