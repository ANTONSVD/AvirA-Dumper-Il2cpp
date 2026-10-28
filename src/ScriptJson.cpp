#include "ScriptJson.hpp"
#include <algorithm>

namespace AvirA
{
	static const char* k_IdaScript = R"AVIRA(# -*- coding: utf-8 -*-
import json

imageBase = idaapi.get_imagebase()


def get_addr(addr):
    return imageBase + addr


def set_name(addr, name):
    if not idc.set_name(addr, name, SN_NOWARN | SN_NOCHECK):
        idc.set_name(addr, name + '_' + str(addr), SN_NOWARN | SN_NOCHECK)


def make_function(start, end):
    next_func = idc.get_next_func(start)
    if next_func < end:
        end = next_func
    if idc.get_func_attr(start, FUNCATTR_START) == start:
        ida_funcs.del_func(start)
    ida_funcs.add_func(start, end)


path = idaapi.ask_file(False, '*.json', 'script.json from AvirA Dumper Il2cpp')
data = json.loads(open(path, 'rb').read().decode('utf-8'))

addresses = data.get('Addresses', [])
for index in range(len(addresses) - 1):
    make_function(get_addr(addresses[index]), get_addr(addresses[index + 1]))

for item in data.get('ScriptMethod', []):
    set_name(get_addr(item['Address']), item['Name'])

string_index = 1
for item in data.get('ScriptString', []):
    addr = get_addr(item['Address'])
    idc.set_name(addr, 'StringLiteral_' + str(string_index), SN_NOWARN | SN_NOCHECK)
    idc.set_cmt(addr, item['Value'], 1)
    string_index += 1

for item in data.get('ScriptMetadata', []):
    addr = get_addr(item['Address'])
    set_name(addr, item['Name'])
    idc.set_cmt(addr, item['Name'], 1)

for item in data.get('ScriptMetadataMethod', []):
    addr = get_addr(item['Address'])
    set_name(addr, item['Name'])
    idc.set_cmt(addr, item['Name'], 1)
    if 'MethodAddress' in item:
        idc.set_cmt(addr, '{0:X}'.format(item['MethodAddress']), 0)

print('Script finished!')
)AVIRA";

	static const char* k_IdaStructScript = R"AVIRA(# -*- coding: utf-8 -*-
import json

imageBase = idaapi.get_imagebase()


def get_addr(addr):
    return imageBase + addr


def set_name(addr, name):
    if not idc.set_name(addr, name, SN_NOWARN | SN_NOCHECK):
        idc.set_name(addr, name + '_' + str(addr), SN_NOWARN | SN_NOCHECK)


def apply_signature(addr, signature):
    if not signature:
        return
    decl = parse_decl(signature, 0)
    if decl is not None:
        apply_type(addr, decl, 1)


def make_function(start, end):
    next_func = idc.get_next_func(start)
    if next_func < end:
        end = next_func
    if idc.get_func_attr(start, FUNCATTR_START) == start:
        ida_funcs.del_func(start)
    ida_funcs.add_func(start, end)


path = idaapi.ask_file(False, '*.json', 'script.json from AvirA Dumper Il2cpp')
hpath = idaapi.ask_file(False, '*.h', 'il2cpp.h from AvirA Dumper Il2cpp')
parse_decls(open(hpath, 'r').read(), 0)
data = json.loads(open(path, 'rb').read().decode('utf-8'))

addresses = data.get('Addresses', [])
for index in range(len(addresses) - 1):
    make_function(get_addr(addresses[index]), get_addr(addresses[index + 1]))

for item in data.get('ScriptMethod', []):
    addr = get_addr(item['Address'])
    set_name(addr, item['Name'])
    apply_signature(addr, item.get('Signature'))

string_index = 1
for item in data.get('ScriptString', []):
    addr = get_addr(item['Address'])
    idc.set_name(addr, 'StringLiteral_' + str(string_index), SN_NOWARN | SN_NOCHECK)
    idc.set_cmt(addr, item['Value'], 1)
    string_index += 1

for item in data.get('ScriptMetadata', []):
    addr = get_addr(item['Address'])
    set_name(addr, item['Name'])
    idc.set_cmt(addr, item['Name'], 1)
    apply_signature(addr, item.get('Signature'))

for item in data.get('ScriptMetadataMethod', []):
    addr = get_addr(item['Address'])
    set_name(addr, item['Name'])
    idc.set_cmt(addr, item['Name'], 1)
    if 'MethodAddress' in item:
        idc.set_cmt(addr, '{0:X}'.format(item['MethodAddress']), 0)

print('Script finished!')
)AVIRA";

	static const char* k_GhidraScript = R"AVIRA(# -*- coding: utf-8 -*-
import json
from ghidra.program.model.symbol import SourceType

baseAddress = currentProgram.getImageBase()
USER_DEFINED = SourceType.USER_DEFINED


def get_addr(addr):
    return baseAddress.add(addr)


def set_name(addr, name):
    try:
        createLabel(addr, name, True, USER_DEFINED)
    except:
        pass


def make_function(start):
    if getFunctionAt(start) is None:
        try:
            createFunction(start, None)
        except:
            pass


f = askFile('script.json from AvirA Dumper Il2cpp', 'Open')
data = json.loads(open(f.getAbsolutePath(), 'rb').read().decode('utf-8'))

for item in data.get('ScriptMethod', []):
    set_name(get_addr(item['Address']), item['Name'])

string_index = 1
for item in data.get('ScriptString', []):
    addr = get_addr(item['Address'])
    set_name(addr, 'StringLiteral_' + str(string_index))
    setEOLComment(addr, item['Value'])
    string_index += 1

for item in data.get('ScriptMetadata', []):
    addr = get_addr(item['Address'])
    set_name(addr, item['Name'])
    setEOLComment(addr, item['Name'])

for item in data.get('ScriptMetadataMethod', []):
    addr = get_addr(item['Address'])
    set_name(addr, item['Name'])
    setEOLComment(addr, item['Name'])

for address in data.get('Addresses', []):
    make_function(get_addr(address))

print('Script finished!')
)AVIRA";

	static void AppendEntry(std::string& target, const std::string& entry)
	{
		if (!target.empty())
			target.push_back(',');
		target += entry;
	}

	static void AppendUnit(std::string& out, u32 unit)
	{
		static const char hex[] = "0123456789abcdef";
		out += "\\u";
		out.push_back(hex[(unit >> 12) & 0xF]);
		out.push_back(hex[(unit >> 8) & 0xF]);
		out.push_back(hex[(unit >> 4) & 0xF]);
		out.push_back(hex[unit & 0xF]);
	}

	static std::string JsonText(const std::string& utf8)
	{
		std::string out = "\"";
		size_t i = 0;
		while (i < utf8.size())
		{
			u32 lead = (u8)utf8[i];
			u32 cp = lead;
			size_t len = 1;
			if (lead >= 0x80)
			{
				if ((lead & 0xE0) == 0xC0 && i + 1 < utf8.size())
				{
					cp = ((lead & 0x1F) << 6) | ((u8)utf8[i + 1] & 0x3F);
					len = 2;
				}
				else if ((lead & 0xF0) == 0xE0 && i + 2 < utf8.size())
				{
					cp = ((lead & 0x0F) << 12) | (((u8)utf8[i + 1] & 0x3F) << 6) | ((u8)utf8[i + 2] & 0x3F);
					len = 3;
				}
				else if ((lead & 0xF8) == 0xF0 && i + 3 < utf8.size())
				{
					cp = ((lead & 0x07) << 18) | (((u8)utf8[i + 1] & 0x3F) << 12) | (((u8)utf8[i + 2] & 0x3F) << 6) | ((u8)utf8[i + 3] & 0x3F);
					len = 4;
				}
				else
					cp = 0xFFFD;
			}
			i += len;
			if (cp == '"')
				out += "\\\"";
			else if (cp == '\\')
				out += "\\\\";
			else if (cp == '\n')
				out += "\\n";
			else if (cp == '\r')
				out += "\\r";
			else if (cp == '\t')
				out += "\\t";
			else if (cp >= 0x20 && cp < 0x7F)
				out.push_back((char)cp);
			else if (cp < 0x10000)
				AppendUnit(out, cp);
			else
			{
				u32 v = cp - 0x10000;
				AppendUnit(out, 0xD800 + (v >> 10));
				AppendUnit(out, 0xDC00 + (v & 0x3FF));
			}
		}
		out.push_back('"');
		return out;
	}

	static std::string LiteralText(const C_Il2cppMetadata& metadata, const S_StringLiteral& literal)
	{
		std::string text;
		if (literal.length > 1048576)
			return text;
		u64 at = metadata.LiteralDataBase() + (u64)literal.offset;
		text.reserve(literal.length);
		for (u32 k = 0; k < literal.length; k++)
			text.push_back((char)metadata.ReadRawU8(at + k));
		return text;
	}

	C_ScriptJson::C_ScriptJson(C_Model& model)
		: m_model(model)
	{
	}

	void C_ScriptJson::CollectUsages()
	{
		const C_Il2cppMetadata& metadata = m_model.Metadata();
		const C_Il2cppBinary& binary = m_model.Binary();
		const std::vector<S_PeSection>& sections = binary.Sections();
		const std::vector<S_StringLiteral>& literals = metadata.StringLiterals();
		const std::vector<S_MethodDef>& methods = metadata.Methods();
		const std::vector<S_FieldRef>& refs = metadata.FieldRefs();
		const std::vector<S_FieldDef>& fields = metadata.Fields();
		const std::vector<S_TypeDef>& typedefs = metadata.Typedefs();
		for (size_t s = 0; s < sections.size(); s++)
		{
			const S_PeSection& section = sections[s];
			if (section.chars != 0x40000040 && section.chars != 0xC0000040)
				continue;
			u64 slot_count = section.raw_size / 8;
			for (u64 k = 0; k < slot_count; k++)
			{
				u64 encoded = binary.BinaryReadU64(section.raw_ptr + k * 8);
				if (encoded == 0 || encoded > 0xFFFFFFFFull || (encoded & 1) == 0)
					continue;
				u32 usage = (u32)((encoded >> 29) & 7);
				u32 index = (u32)((encoded & 0x1FFFFFFEull) >> 1);
				std::string address = std::to_string(section.virt_addr + k * 8);
				if (usage == 5)
				{
					if (index >= literals.size())
						continue;
					AppendEntry(m_strings, "{\"Address\":" + address + ",\"Value\":" + JsonText(LiteralText(metadata, literals[index])) + "}");
				}
				else if (usage == 1 || usage == 2)
				{
					u64 datapoint = 0;
					u32 kind = 0;
					if (!binary.BinaryTypeRaw((int)index, datapoint, kind))
						continue;
					int t = binary.TypedefFromType((int)index);
					bool direct = (kind == 0x11 || kind == 0x12 || kind == 0x55) && t >= 0;
					std::string stem;
					if (direct)
						stem = m_model.Flat(t);
					else
					{
						std::string type_name = binary.TypeName((int)index, -1, -1, 0);
						if (type_name == "?")
							continue;
						stem = m_model.Ident(type_name);
					}
					std::string name = stem + (usage == 1 ? "_TypeInfo" : "_Type");
					std::string signature;
					if (usage == 2)
						signature = "Il2CppType* " + name + ";";
					else if (direct && m_model.IsClass(t))
						signature = stem + "_c* " + name + ";";
					else
						signature = "Il2CppClass* " + name + ";";
					AppendEntry(m_meta, "{\"Address\":" + address + ",\"Name\":" + JsonText(name) + ",\"Signature\":" + JsonText(signature) + "}");
				}
				else if (usage == 3)
				{
					if (index >= methods.size())
						continue;
					const S_MethodDef& method = methods[index];
					if (method.declaring < 0 || (size_t)method.declaring >= typedefs.size())
						continue;
					std::string name = "Method$" + m_model.Flat(method.declaring) + "$$" + m_model.Symbol(metadata.GetString(method.name_idx));
					u64 method_rva = binary.MethodRva(m_model.ImageOf(method.declaring), method.token);
					std::string entry = "{\"Address\":" + address + ",\"Name\":" + JsonText(name);
					if (method_rva != UINT64_MAX)
						entry += ",\"MethodAddress\":" + std::to_string(method_rva);
					AppendEntry(m_meta_methods, entry + "}");
				}
				else if (usage == 4)
				{
					if (index >= refs.size())
						continue;
					const S_FieldRef& ref = refs[index];
					int t = binary.TypedefFromType(ref.type_idx);
					if (t < 0)
						continue;
					const S_TypeDef& type = typedefs[(size_t)t];
					if (ref.field_idx < 0 || ref.field_idx >= type.field_count)
						continue;
					int global = type.field_start + ref.field_idx;
					if (global < 0 || (size_t)global >= fields.size())
						continue;
					std::string name = "Field$" + m_model.Flat(t) + "$$" + m_model.Symbol(metadata.GetString(fields[(size_t)global].name_idx));
					AppendEntry(m_meta, "{\"Address\":" + address + ",\"Name\":" + JsonText(name) + ",\"Signature\":null}");
				}
				else if (usage == 6)
				{
					int def = binary.MethodSpecMethodDef((int)index);
					if (def < 0 || (size_t)def >= methods.size())
						continue;
					const S_MethodDef& method = methods[(size_t)def];
					if (method.declaring < 0 || (size_t)method.declaring >= typedefs.size())
						continue;
					std::string name = "Method$" + m_model.Flat(method.declaring) + "$$" + m_model.Symbol(metadata.GetString(method.name_idx));
					u64 ptr_rva = binary.SpecPointerRva((int)index);
					std::string entry = "{\"Address\":" + address + ",\"Name\":" + JsonText(name);
					if (ptr_rva != UINT64_MAX)
						entry += ",\"MethodAddress\":" + std::to_string(ptr_rva);
					AppendEntry(m_meta_methods, entry + "}");
				}
			}
		}
	}

	std::string C_ScriptJson::MethodSignature(int typedef_index, int method_index, const std::string& name)
	{
		const C_Il2cppMetadata& metadata = m_model.Metadata();
		const S_MethodDef& method = metadata.Methods()[(size_t)method_index];
		std::string text = m_model.CType(method.return_type, 0) + " " + name + "(";
		bool first = true;
		if ((method.flags & 0x0010u) == 0)
		{
			std::string self = m_model.IsEnum(typedef_index) ? std::string("void*") : m_model.Flat(typedef_index) + "_o*";
			text += self + " __this";
			first = false;
		}
		for (int p = 0; p < method.param_count; p++)
		{
			int global = method.param_start + p;
			if (global < 0 || (size_t)global >= metadata.Params().size())
				break;
			const S_ParamDef& param = metadata.Params()[(size_t)global];
			std::string param_name = m_model.Ident(metadata.GetString(param.name_idx));
			if (param_name == "method" || param_name == "__this")
				param_name += "_";
			if (!first)
				text += ", ";
			first = false;
			text += m_model.CType(param.type_idx, 0) + " " + param_name;
		}
		if (!first)
			text += ", ";
		text += "const MethodInfo* method);";
		return text;
	}

	void C_ScriptJson::CollectMethods()
	{
		const C_Il2cppMetadata& metadata = m_model.Metadata();
		const C_Il2cppBinary& binary = m_model.Binary();
		const std::vector<S_ImageDef>& images = metadata.Images();
		const std::vector<S_TypeDef>& typedefs = metadata.Typedefs();
		const std::vector<S_MethodDef>& methods = metadata.Methods();
		for (size_t i = 0; i < images.size(); i++)
		{
			int end = images[i].start + (int)images[i].count;
			for (int t = images[i].start; t < end && (size_t)t < typedefs.size(); t++)
			{
				const S_TypeDef& type = typedefs[(size_t)t];
				for (int k = 0; k < type.method_count; k++)
				{
					int global = type.method_start + k;
					if (global < 0 || (size_t)global >= methods.size())
						break;
					const S_MethodDef& method = methods[(size_t)global];
					std::string name = m_model.Flat(t) + "$$" + m_model.Symbol(metadata.GetString(method.name_idx));
					u64 rva = binary.MethodRva(images[i].name, method.token);
					if (rva != UINT64_MAX && rva != 0)
					{
						AppendEntry(m_methods, "{\"Address\":" + std::to_string(rva) + ",\"Name\":" + JsonText(name)
							+ ",\"Signature\":" + JsonText(MethodSignature(t, global, name)) + "}");
						m_addresses.push_back(rva);
					}
					std::vector<S_GenericInstance> instances = binary.GenericInstancesFull(global);
					for (size_t n = 0; n < instances.size(); n++)
					{
						const S_GenericInstance& instance = instances[n];
						if (instance.rva == UINT64_MAX || instance.rva == 0)
							continue;
						size_t cut = instance.display_name.size() > instance.method_name.size()
							? instance.display_name.size() - instance.method_name.size() - 1 : 0;
						std::string generic_name = m_model.Symbol(instance.display_name.substr(0, cut)) + "$$" + m_model.Symbol(instance.method_name);
						AppendEntry(m_methods, "{\"Address\":" + std::to_string(instance.rva) + ",\"Name\":" + JsonText(generic_name)
							+ ",\"Signature\":null}");
						m_addresses.push_back(instance.rva);
					}
				}
			}
		}
	}

	bool C_ScriptJson::Write(const std::string& directory)
	{
		CollectUsages();
		CollectMethods();
		std::sort(m_addresses.begin(), m_addresses.end());
		m_addresses.erase(std::unique(m_addresses.begin(), m_addresses.end()), m_addresses.end());
		std::string addresses;
		for (size_t i = 0; i < m_addresses.size(); i++)
		{
			if (m_addresses[i] != 0)
				AppendEntry(addresses, std::to_string(m_addresses[i]));
		}
		std::string json = "{\"ScriptMethod\":[" + m_methods + "],\"ScriptString\":[" + m_strings + "],\"ScriptMetadata\":["
			+ m_meta + "],\"ScriptMetadataMethod\":[" + m_meta_methods + "],\"Addresses\":[" + addresses + "]}\n";
		if (!WriteTextFile(directory + "\\script.json", json))
			return false;

		const C_Il2cppMetadata& metadata = m_model.Metadata();
		const std::vector<S_StringLiteral>& literals = metadata.StringLiterals();
		std::string list;
		for (size_t i = 0; i < literals.size(); i++)
		{
			if (!list.empty())
				list += ",\n";
			list += "\t{\"Index\":" + std::to_string(i) + ",\"Value\":" + JsonText(LiteralText(metadata, literals[i])) + "}";
		}
		if (!WriteTextFile(directory + "\\stringliteral.json", "[\n" + list + "\n]\n"))
			return false;
		if (!WriteTextFile(directory + "\\ida.py", k_IdaScript))
			return false;
		if (!WriteTextFile(directory + "\\ida_with_struct.py", k_IdaStructScript))
			return false;
		return WriteTextFile(directory + "\\ghidra.py", k_GhidraScript);
	}
}
