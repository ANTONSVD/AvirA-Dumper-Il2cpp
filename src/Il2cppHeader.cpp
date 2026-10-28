#include "Il2cppHeader.hpp"

namespace AvirA
{
	static const char* k_BaseTypes =
		"typedef struct Il2CppClass Il2CppClass;\n"
		"typedef struct Il2CppType Il2CppType;\n"
		"typedef struct MethodInfo MethodInfo;\n"
		"\n"
		"struct Il2CppObject\n"
		"{\n"
		"\tIl2CppClass* klass;\n"
		"\tvoid* monitor;\n"
		"};\n"
		"\n";

	C_Il2cppHeader::C_Il2cppHeader(C_Model& model)
		: m_model(model)
	{
	}

	bool C_Il2cppHeader::Write(const std::string& path)
	{
		const std::vector<S_TypeDef>& typedefs = m_model.Metadata().Typedefs();
		std::string out = k_BaseTypes;
		for (size_t t = 0; t < typedefs.size(); t++)
		{
			int index = (int)t;
			if (m_model.IsEnum(index))
				continue;
			std::string flat = m_model.Flat(index);
			if (m_model.IsClass(index))
				out += "struct " + flat + "_c;\n";
			out += "struct " + flat + "_o;\n";
		}
		out += "\n";
		for (size_t t = 0; t < typedefs.size(); t++)
		{
			if (m_model.IsValueType((int)t))
				EmitValueType((int)t, out);
		}
		for (size_t t = 0; t < typedefs.size(); t++)
		{
			if (m_model.IsClass((int)t))
				EmitClass((int)t, out);
		}
		return WriteTextFile(path, out);
	}

	void C_Il2cppHeader::EmitValueType(int typedef_index, std::string& out)
	{
		if (m_done.count(typedef_index) != 0 || m_busy.count(typedef_index) != 0)
			return;
		m_busy.insert(typedef_index);
		const S_Layout& layout = m_model.Layout(typedef_index);
		for (size_t i = 0; i < layout.members.size(); i++)
		{
			const S_Member& member = layout.members[i];
			if (member.opaque)
				continue;
			int dependency = m_model.ByValueTypedef(member.type_index);
			if (dependency >= 0 && dependency != typedef_index)
				EmitValueType(dependency, out);
		}
		std::string flat = m_model.Flat(typedef_index);
		out += "struct " + flat + "_o\n{\n";
		if (layout.members.empty())
			out += "\tuint8_t _empty;\n";
		else
			EmitMembers(layout, false, out);
		out += "};\n\n";
		m_busy.erase(typedef_index);
		m_done.insert(typedef_index);
	}

	void C_Il2cppHeader::EmitClass(int typedef_index, std::string& out)
	{
		const S_Layout& layout = m_model.Layout(typedef_index);
		std::string flat = m_model.Flat(typedef_index);
		out += "struct " + flat + "_o\n{\n\t" + flat + "_c* klass;\n\tvoid* monitor;\n";
		EmitMembers(layout, true, out);
		out += "};\n\n";
	}

	void C_Il2cppHeader::EmitMembers(const S_Layout& layout, bool is_class, std::string& out) const
	{
		u64 cursor = is_class ? 16 : 0;
		for (size_t i = 0; i < layout.members.size(); i++)
		{
			const S_Member& member = layout.members[i];
			if (member.offset > cursor)
			{
				out += "\tuint8_t _pad_" + FormatHexUpper(cursor) + "[" + FormatU64(member.offset - cursor) + "];\n";
				cursor = member.offset;
			}
			if (member.opaque)
				out += "\tuint8_t " + member.name + "[" + FormatU64(member.size) + "];\n";
			else
				out += "\t" + member.ctype + " " + member.name + ";\n";
			if (member.offset + member.size > cursor)
				cursor = member.offset + member.size;
		}
	}
}
