#include "DumpWriter.hpp"

namespace AvirA
{
	C_DumpWriter::C_DumpWriter(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary)
		: m_metadata(metadata), m_binary(binary), m_attributes(metadata, binary)
	{
	}

	std::string C_DumpWriter::TypeAccess(u32 visibility) const
	{
		switch (visibility)
		{
		case 1:
		case 2:
			return "public ";
		case 0:
		case 6:
		case 5:
			return "internal ";
		case 3:
			return "private ";
		case 4:
			return "protected ";
		case 7:
			return "protected internal ";
		default:
			return "";
		}
	}

	std::string C_DumpWriter::MethodAccess(u32 access) const
	{
		switch (access)
		{
		case 1:
			return "private ";
		case 6:
			return "public ";
		case 4:
			return "protected ";
		case 3:
		case 2:
			return "internal ";
		case 5:
			return "protected internal ";
		default:
			return "";
		}
	}

	std::string C_DumpWriter::FieldAccess(u32 access) const
	{
		return MethodAccess(access);
	}

	std::string C_DumpWriter::MethodModifiers(u32 flags) const
	{
		std::string out = MethodAccess(flags & 0x07u);
		if (flags & 0x10u)
			out += "static ";
		if (flags & 0x400u)
		{
			out += "abstract ";
			if ((flags & 0x100u) == 0)
				out += "override ";
		}
		else if (flags & 0x20u)
		{
			if ((flags & 0x100u) == 0)
				out += "sealed override ";
		}
		else if (flags & 0x40u)
		{
			if (flags & 0x100u)
				out += "virtual ";
			else
				out += "override ";
		}
		if (flags & 0x2000u)
			out += "extern ";
		return out;
	}

	std::string C_DumpWriter::MethodGenericParams(int container_idx) const
	{
		if (container_idx < 0 || (size_t)container_idx >= m_metadata.GenericContainers().size())
			return "";
		const S_GenericContainer& gc = m_metadata.GenericContainers()[(size_t)container_idx];
		const std::vector<S_GenericParam>& gparams = m_metadata.GenericParams();
		std::string out = "<";
		for (int i = 0; i < gc.argc; i++)
		{
			if (i > 0)
				out += ", ";
			size_t global = (size_t)gc.param_start + i;
			if (global < gparams.size())
				out += m_metadata.GetString(gparams[global].name_idx);
		}
		out += ">";
		return out;
	}

	void C_DumpWriter::WriteTypedef(int image_index, int typedef_index, std::string& body)
	{
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		const std::vector<S_MethodDef>& methods = m_metadata.Methods();
		const std::vector<S_ParamDef>& params = m_metadata.Params();
		const std::vector<S_FieldDef>& fields = m_metadata.Fields();
		const std::vector<S_PropertyDef>& properties = m_metadata.Properties();
		const std::vector<S_ImageDef>& images = m_metadata.Images();
		if (typedef_index < 0 || (size_t)typedef_index >= typedefs.size())
			return;
		const S_TypeDef& type = typedefs[(size_t)typedef_index];

		std::string extends;
		if (type.parent >= 0)
		{
			std::string parent_name = m_binary.TypeName(type.parent, typedef_index, -1);
			if (!type.is_valuetype && !type.is_enum && parent_name != "object")
				extends = parent_name;
		}
		for (int i = 0; i < type.interfaces_count; i++)
		{
			size_t at = (size_t)type.interfaces_start + i;
			if (at >= m_metadata.InterfaceIndices().size())
				break;
			std::string iname = m_binary.TypeName(m_metadata.InterfaceIndices()[at], typedef_index, -1);
			if (!extends.empty())
				extends += ", ";
			extends += iname;
		}

		std::string head = "\n// Namespace: " + m_metadata.GetString(type.namespace_idx) + "\n";
		head += m_attributes.CustomAttributes(image_index, type.token, "");
		if (type.flags & 0x2000u)
			head += "[Serializable]\n";
		head += TypeAccess(type.flags & 0x07u);
		if ((type.flags & 0x80u) && (type.flags & 0x100u))
			head += "static ";
		else if (!(type.flags & 0x20u) && (type.flags & 0x80u))
			head += "abstract ";
		else if (!type.is_valuetype && !type.is_enum && (type.flags & 0x100u))
			head += "sealed ";
		if (type.flags & 0x20u)
			head += "interface ";
		else if (type.is_enum)
			head += "enum ";
		else if (type.is_valuetype)
			head += "struct ";
		else
			head += "class ";
		head += m_binary.TypedefDeclName(typedef_index);
		if (!extends.empty())
			head += " : " + extends;
		head += " // TypeDefIndex: " + FormatI32(typedef_index) + "\n";

		bool empty = (type.field_count == 0 && type.property_count == 0 && type.method_count == 0);
		if (empty)
		{
			body += head + "{}\n";
			return;
		}
		body += head + "{\n";
		bool first_section = true;

		if (type.field_count > 0)
		{
			if (!first_section)
				body += "\n";
			first_section = false;
			body += "\t// Fields\n";
			for (int i = 0; i < type.field_count; i++)
			{
				int global = type.field_start + i;
				if (global < 0 || (size_t)global >= fields.size())
					break;
				const S_FieldDef& field = fields[(size_t)global];
				body += m_attributes.CustomAttributes(image_index, field.token, "\t");
				u32 fattrs = m_binary.BinaryTypeAttrs(field.type_idx);
				bool is_const = (fattrs & 0x40u) != 0;
				bool is_static = (fattrs & 0x10u) != 0;
				body += "\t" + FieldAccess(fattrs & 0x07u);
				if (is_const)
					body += "const ";
				else
				{
					if (is_static)
						body += "static ";
					if (fattrs & 0x20u)
						body += "readonly ";
				}
				body += m_binary.TypeName(field.type_idx, typedef_index, -1);
				body += " " + m_metadata.GetString(field.name_idx);
				S_DefaultValue default_value;
				if (m_metadata.FieldDefault(global, default_value) && default_value.data_idx != -1)
				{
					std::string value_text;
					u64 fail_ptr = 0;
					if (m_attributes.DefaultValueText(default_value.type_idx, default_value.data_idx, value_text, fail_ptr))
						body += " = " + value_text;
					else
						body += " /*Metadata offset 0x" + FormatHexUpper(fail_ptr) + "*/";
				}
				if (!is_const)
				{
					i32 offset = m_binary.FieldOffset(typedef_index, i, type.is_valuetype, is_static);
					body += "; // 0x" + FormatHexUpper((u64)(u32)offset) + "\n";
				}
				else
					body += ";\n";
			}
		}

		if (type.property_count > 0)
		{
			if (!first_section)
				body += "\n";
			first_section = false;
			body += "\t// Properties\n";
			for (int i = 0; i < type.property_count; i++)
			{
				int global = type.property_start + i;
				if (global < 0 || (size_t)global >= properties.size())
					break;
				const S_PropertyDef& prop = properties[(size_t)global];
				body += m_attributes.CustomAttributes(image_index, prop.token, "\t");
				body += "\t";
				if (prop.get >= 0)
				{
					int method_global = type.method_start + prop.get;
					if (method_global >= 0 && (size_t)method_global < methods.size())
					{
						const S_MethodDef& getter = methods[(size_t)method_global];
						body += MethodModifiers(getter.flags);
						body += m_binary.TypeName(getter.return_type, typedef_index, method_global);
						body += " " + m_metadata.GetString(prop.name_idx) + " { ";
					}
				}
				else if (prop.set >= 0)
				{
					int method_global = type.method_start + prop.set;
					if (method_global >= 0 && (size_t)method_global < methods.size())
					{
						const S_MethodDef& setter = methods[(size_t)method_global];
						body += MethodModifiers(setter.flags);
						std::string ptype = "?";
						if (setter.param_count > 0)
						{
							size_t param_at = (size_t)setter.param_start;
							if (param_at < params.size())
								ptype = m_binary.TypeName(params[param_at].type_idx, typedef_index, method_global);
						}
						body += ptype + " " + m_metadata.GetString(prop.name_idx) + " { ";
					}
				}
				if (prop.get >= 0)
					body += "get; ";
				if (prop.set >= 0)
					body += "set; ";
				body += "}\n";
			}
		}

		if (type.method_count > 0)
		{
			if (!first_section)
				body += "\n";
			first_section = false;
			body += "\t// Methods\n";
			for (int i = 0; i < type.method_count; i++)
			{
				int global = type.method_start + i;
				if (global < 0 || (size_t)global >= methods.size())
					break;
				const S_MethodDef& method = methods[(size_t)global];
				body += "\n";
				body += m_attributes.CustomAttributes(image_index, method.token, "\t");
				bool is_abstract = (method.flags & 0x400u) != 0;
				u64 pointer = m_binary.MethodRva(images[(size_t)image_index].name, method.token);
				if (!is_abstract && pointer != 0 && pointer != UINT64_MAX)
				{
					u64 file_offset = m_binary.FileOffsetFromRva(pointer);
					if (file_offset == UINT64_MAX)
						file_offset = 0;
					body += "\t// RVA: 0x" + FormatHexUpper(pointer)
						+ " Offset: 0x" + FormatHexUpper(file_offset)
						+ " VA: 0x" + FormatHexUpper(pointer + m_binary.ImageBase());
				}
				else
					body += "\t// RVA: -1 Offset: -1";
				if (method.slot != 0xFFFFu)
					body += " Slot: " + FormatU32(method.slot & 0xFFFFu);
				body += "\n";
				body += "\t" + MethodModifiers(method.flags);
				std::string method_name = m_metadata.GetString(method.name_idx);
				if (method.generic_container >= 0)
					method_name += MethodGenericParams(method.generic_container);
				if (m_binary.BinaryTypeByref(method.return_type))
					body += "ref ";
				body += m_binary.TypeName(method.return_type, typedef_index, global);
				body += " " + method_name + "(";
				for (int j = 0; j < method.param_count; j++)
				{
					if (j > 0)
						body += ", ";
					int param_global = method.param_start + j;
					std::string param_text;
					if (param_global >= 0 && (size_t)param_global < params.size())
					{
						const S_ParamDef& param = params[(size_t)param_global];
						u32 pattrs = m_binary.BinaryTypeAttrs(param.type_idx);
						std::string pname = m_metadata.GetString(param.name_idx);
						std::string ptype = m_binary.TypeName(param.type_idx, typedef_index, global);
						if (m_binary.BinaryTypeByref(param.type_idx))
						{
							if ((pattrs & 0x02u) && !(pattrs & 0x01u))
								param_text += "out ";
							else if (!(pattrs & 0x02u) && (pattrs & 0x01u))
								param_text += "in ";
							else
								param_text += "ref ";
						}
						else
						{
							if (pattrs & 0x01u)
								param_text += "[In] ";
							if (pattrs & 0x02u)
								param_text += "[Out] ";
						}
						param_text += ptype + " " + pname;
						S_DefaultValue default_value;
						if (m_metadata.ParamDefault(param_global, default_value) && default_value.data_idx != -1)
						{
							std::string value_text;
							u64 fail_ptr = 0;
							if (m_attributes.DefaultValueText(default_value.type_idx, default_value.data_idx, value_text, fail_ptr))
								param_text += " = " + value_text;
							else
								param_text += " /*Metadata offset 0x" + FormatHexUpper(fail_ptr) + "*/";
						}
					}
					body += param_text;
				}
				if (is_abstract)
					body += ");\n";
				else
					body += ") { }\n";

				std::vector<S_GenericInstance> instances = m_binary.GenericInstancesFull(global);
				if (!instances.empty())
				{
					std::vector<u64> order;
					std::vector<std::vector<size_t>> groups;
					for (size_t k = 0; k < instances.size(); k++)
					{
						size_t found = (size_t)-1;
						for (size_t g = 0; g < order.size(); g++)
						{
							if (order[g] == instances[k].pointer)
							{
								found = g;
								break;
							}
						}
						if (found == (size_t)-1)
						{
							order.push_back(instances[k].pointer);
							groups.push_back(std::vector<size_t>());
							found = groups.size() - 1;
						}
						groups[found].push_back(k);
					}
					body += "\t/* GenericInstMethod :\n";
					for (size_t g = 0; g < order.size(); g++)
					{
						body += "\t|\n";
						if (order[g] != 0)
						{
							u64 rva = order[g] - m_binary.ImageBase();
							u64 file_offset = m_binary.FileOffsetFromRva(rva);
							if (file_offset == UINT64_MAX)
								file_offset = 0;
							body += "\t|-RVA: 0x" + FormatHexUpper(rva)
								+ " Offset: 0x" + FormatHexUpper(file_offset)
								+ " VA: 0x" + FormatHexUpper(order[g]) + "\n";
						}
						else
							body += "\t|-RVA: -1 Offset: -1\n";
						for (size_t k = 0; k < groups[g].size(); k++)
							body += "\t|-" + instances[groups[g][k]].display_name + "\n";
					}
					body += "\t*/\n";
				}
			}
		}

		body += "}\n";
	}

	bool C_DumpWriter::Write(const std::string& path)
	{
		const std::vector<S_ImageDef>& images = m_metadata.Images();
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		std::string body;
		body.reserve(35000000);
		for (size_t i = 0; i < images.size(); i++)
			body += "// Image " + FormatU32((u32)i) + ": " + images[i].name + " - " + FormatI32(images[i].start) + "\n";
		for (size_t i = 0; i < images.size(); i++)
		{
			if (i % 10 == 0 || i + 1 == images.size())
				C_Log::Info("Dump image " + FormatU32((u32)(i + 1)) + "/" + FormatU32((u32)images.size()) + ": " + images[i].name);
			int type_end = images[i].start + (int)images[i].count;
			for (int t = images[i].start; t < type_end; t++)
			{
				if (t < 0 || (size_t)t >= typedefs.size())
					break;
				WriteTypedef((int)i, t, body);
			}
		}
		FILE* file = nullptr;
		if (fopen_s(&file, path.c_str(), "wb") != 0 || !file)
			return false;
		fwrite(body.data(), 1, body.size(), file);
		fclose(file);
		return true;
	}
}
