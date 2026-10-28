#include "Model.hpp"
#include <algorithm>

namespace AvirA
{
	static const char* k_Keywords[] =
	{
		"auto", "bool", "break", "case", "catch", "char", "class", "const", "continue", "default", "delete", "do",
		"double", "else", "enum", "explicit", "extern", "false", "float", "for", "friend", "goto", "if", "inline",
		"int", "long", "mutable", "namespace", "new", "operator", "private", "protected", "public", "register",
		"return", "short", "signed", "sizeof", "static", "struct", "switch", "template", "this", "throw", "true",
		"try", "typedef", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "while"
	};

	static bool IsKeyword(const std::string& text)
	{
		for (size_t i = 0; i < sizeof(k_Keywords) / sizeof(k_Keywords[0]); i++)
		{
			if (text == k_Keywords[i])
				return true;
		}
		return false;
	}

	static u64 RoundUp(u64 value, u64 align)
	{
		if (align <= 1)
			return value;
		return (value + align - 1) / align * align;
	}

	C_Model::C_Model(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary)
		: m_metadata(metadata), m_binary(binary)
	{
		m_used.insert("Il2CppObject");
		m_used.insert("Il2CppClass");
		m_used.insert("Il2CppType");
		m_used.insert("MethodInfo");
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		for (size_t t = 0; t < typedefs.size(); t++)
		{
			if (m_metadata.GetString(typedefs[t].name_idx) == "String" && m_metadata.GetString(typedefs[t].namespace_idx) == "System")
			{
				m_string_typedef = (int)t;
				break;
			}
		}
	}

	const C_Il2cppMetadata& C_Model::Metadata() const
	{
		return m_metadata;
	}

	const C_Il2cppBinary& C_Model::Binary() const
	{
		return m_binary;
	}

	std::string C_Model::Ident(const std::string& text) const
	{
		std::string out;
		out.reserve(text.size() + 1);
		for (size_t i = 0; i < text.size(); i++)
		{
			char c = text[i];
			bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
			out.push_back(ok ? c : '_');
		}
		if (out.empty() || (out[0] >= '0' && out[0] <= '9'))
			out.insert(out.begin(), '_');
		if (IsKeyword(out))
			out.push_back('_');
		return out;
	}

	std::string C_Model::Symbol(const std::string& text) const
	{
		std::string out;
		out.reserve(text.size() + 1);
		for (size_t i = 0; i < text.size(); i++)
		{
			char c = text[i];
			bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$';
			out.push_back(ok ? c : '_');
		}
		if (out.empty())
			out = "_";
		return out;
	}

	std::string C_Model::Unique(const std::string& base)
	{
		std::string name = base;
		int index = 1;
		while (m_used.count(name) != 0)
			name = base + "_" + std::to_string(++index);
		m_used.insert(name);
		return name;
	}

	int C_Model::DeclaringOf(int typedef_index) const
	{
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		if (typedef_index < 0 || (size_t)typedef_index >= typedefs.size())
			return -1;
		int declaring = typedefs[(size_t)typedef_index].declaring;
		if (declaring < 0 || declaring == typedef_index || (size_t)declaring >= typedefs.size())
			return -1;
		return declaring;
	}

	std::string C_Model::Flat(int typedef_index)
	{
		auto found = m_flat.find(typedef_index);
		if (found != m_flat.end())
			return found->second;
		const S_TypeDef& type = m_metadata.Typedefs()[(size_t)typedef_index];
		std::string name = m_metadata.GetString(type.name_idx);
		size_t tick = name.find('`');
		if (tick != std::string::npos)
			name = name.substr(0, tick);
		std::string base;
		int outer = DeclaringOf(typedef_index);
		if (outer >= 0)
			base = Flat(outer) + "_" + Ident(name);
		else
		{
			std::string ns = m_metadata.GetString(type.namespace_idx);
			base = Ident(ns.empty() ? name : ns + "_" + name);
		}
		std::string unique = Unique(base);
		m_flat[typedef_index] = unique;
		return unique;
	}

	bool C_Model::IsEnum(int typedef_index) const
	{
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		return typedef_index >= 0 && (size_t)typedef_index < typedefs.size() && typedefs[(size_t)typedef_index].is_enum;
	}

	bool C_Model::IsValueType(int typedef_index) const
	{
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		return typedef_index >= 0 && (size_t)typedef_index < typedefs.size()
			&& typedefs[(size_t)typedef_index].is_valuetype && !typedefs[(size_t)typedef_index].is_enum;
	}

	bool C_Model::IsClass(int typedef_index) const
	{
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		return typedef_index >= 0 && (size_t)typedef_index < typedefs.size()
			&& !typedefs[(size_t)typedef_index].is_valuetype && !typedefs[(size_t)typedef_index].is_enum;
	}

	std::string C_Model::ImageOf(int typedef_index) const
	{
		const std::vector<S_ImageDef>& images = m_metadata.Images();
		for (size_t i = 0; i < images.size(); i++)
		{
			int end = images[i].start + (int)images[i].count;
			if (typedef_index >= images[i].start && typedef_index < end)
				return images[i].name;
		}
		return "";
	}

	int C_Model::ByValueTypedef(int type_index) const
	{
		u64 datapoint = 0;
		u32 kind = 0;
		if (!m_binary.BinaryTypeRaw(type_index, datapoint, kind) || kind != 0x11)
			return -1;
		int t = m_binary.TypedefFromType(type_index);
		if (!IsValueType(t))
			return -1;
		return t;
	}

	std::string C_Model::CType(int type_index, int depth)
	{
		u64 datapoint = 0;
		u32 kind = 0;
		if (depth > 8 || !m_binary.BinaryTypeRaw(type_index, datapoint, kind))
			return "void*";
		switch (kind)
		{
		case 0x01:
			return "void";
		case 0x02:
			return "bool";
		case 0x03:
		case 0x07:
			return "uint16_t";
		case 0x04:
			return "int8_t";
		case 0x05:
			return "uint8_t";
		case 0x06:
			return "int16_t";
		case 0x08:
			return "int32_t";
		case 0x09:
			return "uint32_t";
		case 0x0A:
			return "int64_t";
		case 0x0B:
			return "uint64_t";
		case 0x0C:
			return "float";
		case 0x0D:
			return "double";
		case 0x0E:
			return m_string_typedef >= 0 ? Flat(m_string_typedef) + "_o*" : std::string("Il2CppObject*");
		case 0x18:
			return "intptr_t";
		case 0x19:
			return "uintptr_t";
		case 0x0F:
		case 0x10:
			return "void*";
		case 0x11:
		{
			int t = m_binary.TypedefFromType(type_index);
			if (t < 0)
				return "void*";
			if (IsEnum(t))
				return CType(m_metadata.Typedefs()[(size_t)t].element, depth + 1);
			return Flat(t) + "_o";
		}
		case 0x12:
		case 0x55:
		{
			int t = m_binary.TypedefFromType(type_index);
			if (t < 0)
				return "Il2CppObject*";
			return Flat(t) + "_o*";
		}
		case 0x13:
		case 0x14:
		case 0x1C:
		case 0x1D:
		case 0x1E:
			return "Il2CppObject*";
		case 0x15:
		{
			int t = m_binary.TypedefFromType(type_index);
			if (IsClass(t))
				return "Il2CppObject*";
			return "void*";
		}
		case 0x16:
		default:
			return "void*";
		}
	}

	u64 C_Model::TypeSize(int type_index, int depth)
	{
		u64 datapoint = 0;
		u32 kind = 0;
		if (depth > 8 || !m_binary.BinaryTypeRaw(type_index, datapoint, kind))
			return 8;
		switch (kind)
		{
		case 0x02:
		case 0x04:
		case 0x05:
			return 1;
		case 0x03:
		case 0x06:
		case 0x07:
			return 2;
		case 0x08:
		case 0x09:
		case 0x0C:
			return 4;
		case 0x16:
			return 16;
		case 0x11:
		{
			int t = m_binary.TypedefFromType(type_index);
			if (t < 0)
				return 8;
			if (IsEnum(t))
				return TypeSize(m_metadata.Typedefs()[(size_t)t].element, depth + 1);
			const S_Layout& layout = Layout(t);
			return layout.busy ? 8 : layout.size;
		}
		case 0x15:
		{
			int t = m_binary.TypedefFromType(type_index);
			if (!IsValueType(t))
				return 8;
			const S_Layout& layout = Layout(t);
			return layout.busy ? 8 : layout.size;
		}
		default:
			return 8;
		}
	}

	u64 C_Model::TypeAlign(int type_index, int depth)
	{
		u64 datapoint = 0;
		u32 kind = 0;
		if (depth > 8 || !m_binary.BinaryTypeRaw(type_index, datapoint, kind))
			return 8;
		switch (kind)
		{
		case 0x02:
		case 0x04:
		case 0x05:
			return 1;
		case 0x03:
		case 0x06:
		case 0x07:
			return 2;
		case 0x08:
		case 0x09:
		case 0x0C:
			return 4;
		case 0x11:
		case 0x15:
		{
			int t = m_binary.TypedefFromType(type_index);
			if (t < 0)
				return 8;
			if (IsEnum(t))
				return TypeAlign(m_metadata.Typedefs()[(size_t)t].element, depth + 1);
			if (!IsValueType(t))
				return 8;
			const S_Layout& layout = Layout(t);
			return layout.busy ? 8 : layout.align;
		}
		default:
			return 8;
		}
	}

	const S_Layout& C_Model::Layout(int typedef_index)
	{
		S_Layout& layout = m_layouts[typedef_index];
		if (layout.done || layout.busy)
			return layout;
		layout.busy = true;
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		const std::vector<S_FieldDef>& fields = m_metadata.Fields();
		bool value_type = IsValueType(typedef_index);
		std::vector<S_Member> members;
		int owner = typedef_index;
		int guard = 0;
		while (owner >= 0 && guard < 64)
		{
			guard++;
			const S_TypeDef& type = typedefs[(size_t)owner];
			for (int i = 0; i < type.field_count; i++)
			{
				int global = type.field_start + i;
				if (global < 0 || (size_t)global >= fields.size())
					break;
				const S_FieldDef& field = fields[(size_t)global];
				u32 attrs = m_binary.BinaryTypeAttrs(field.type_idx);
				if ((attrs & 0x0010u) != 0 || (attrs & 0x0040u) != 0)
					continue;
				i32 offset = m_binary.FieldOffset(owner, i, type.is_valuetype, false);
				if (offset < 0)
					continue;
				S_Member member;
				member.name = Ident(m_metadata.GetString(field.name_idx));
				member.type_index = field.type_idx;
				member.offset = (u64)offset;
				u64 datapoint = 0;
				u32 kind = 0;
				if (m_binary.BinaryTypeRaw(field.type_idx, datapoint, kind) && kind == 0x15)
					member.opaque = IsValueType(m_binary.TypedefFromType(field.type_idx));
				member.size = TypeSize(field.type_idx, 0);
				member.ctype = member.opaque ? std::string("uint8_t") : CType(field.type_idx, 0);
				members.push_back(member);
			}
			owner = type.parent >= 0 ? m_binary.TypedefFromType(type.parent) : -1;
		}

		std::stable_sort(members.begin(), members.end(), [](const S_Member& a, const S_Member& b)
		{
			return a.offset < b.offset;
		});
		std::unordered_set<std::string> names;
		for (size_t i = 0; i < members.size(); i++)
		{
			std::string base = members[i].name;
			std::string name = base;
			if (names.count(name) != 0)
				name = base + "_" + FormatHexUpper(members[i].offset);
			int suffix = 1;
			std::string stem = name;
			while (names.count(name) != 0)
				name = stem + "_" + std::to_string(++suffix);
			names.insert(name);
			members[i].name = name;
		}

		u64 end = 0;
		u64 align = 1;
		for (size_t i = 0; i < members.size(); i++)
		{
			u64 member_end = members[i].offset + members[i].size;
			if (member_end > end)
				end = member_end;
			u64 member_align = TypeAlign(members[i].type_index, 0);
			if (member_align > align)
				align = member_align;
		}
		layout.members = members;
		layout.align = value_type ? align : 1;
		if (value_type)
			layout.size = members.empty() ? 1 : RoundUp(end, align);
		else
			layout.size = 0;
		layout.done = true;
		layout.busy = false;
		return layout;
	}
}
