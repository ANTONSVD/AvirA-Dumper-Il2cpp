#pragma once
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include "Il2cppBinary.hpp"

namespace AvirA
{
	struct S_Member
	{
		std::string name;
		std::string ctype;
		int type_index = -1;
		u64 offset = 0;
		u64 size = 0;
		bool opaque = false;
	};

	struct S_Layout
	{
		std::vector<S_Member> members;
		u64 size = 0;
		u64 align = 1;
		bool done = false;
		bool busy = false;
	};

	inline bool WriteTextFile(const std::string& path, const std::string& text)
	{
		FILE* file = nullptr;
		if (fopen_s(&file, path.c_str(), "wb") != 0 || !file)
			return false;
		size_t put = std::fwrite(text.data(), 1, text.size(), file);
		std::fclose(file);
		return put == text.size();
	}

	class C_Model
	{
	public:
		C_Model(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary);

		const C_Il2cppMetadata& Metadata() const;
		const C_Il2cppBinary& Binary() const;

		std::string Ident(const std::string& text) const;
		std::string Symbol(const std::string& text) const;
		std::string Flat(int typedef_index);
		std::string CType(int type_index, int depth = 0);
		std::string ImageOf(int typedef_index) const;
		int DeclaringOf(int typedef_index) const;
		int ByValueTypedef(int type_index) const;
		bool IsClass(int typedef_index) const;
		bool IsValueType(int typedef_index) const;
		bool IsEnum(int typedef_index) const;
		const S_Layout& Layout(int typedef_index);

	private:
		u64 TypeSize(int type_index, int depth);
		u64 TypeAlign(int type_index, int depth);
		std::string Unique(const std::string& base);

		const C_Il2cppMetadata& m_metadata;
		const C_Il2cppBinary& m_binary;
		int m_string_typedef = -1;
		std::unordered_map<int, std::string> m_flat;
		std::unordered_set<std::string> m_used;
		std::unordered_map<int, S_Layout> m_layouts;
	};
}
