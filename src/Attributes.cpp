#include "Attributes.hpp"
#include <charconv>
#include <limits>

namespace AvirA
{
	C_Attributes::C_Attributes(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary)
		: m_metadata(metadata), m_binary(binary)
	{
	}

	static void AppendChar(std::string& out, unsigned value)
	{
		if (value < 0x80)
			out.push_back((char)value);
		else if (value < 0x800)
		{
			out.push_back((char)(0xC0 | (value >> 6)));
			out.push_back((char)(0x80 | (value & 0x3F)));
		}
		else
		{
			out.push_back((char)(0xE0 | (value >> 12)));
			out.push_back((char)(0x80 | ((value >> 6) & 0x3F)));
			out.push_back((char)(0x80 | (value & 0x3F)));
		}
	}

	std::string C_Attributes::EscapeDumpString(const std::string& text)
	{
		std::string out;
		for (size_t i = 0; i < text.size(); i++)
		{
			unsigned char c = (unsigned char)text[i];
			switch (c)
			{
			case '\'': out += "\\'"; break;
			case '"': out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case 0: out += "\\0"; break;
			case 7: out += "\\a"; break;
			case 8: out += "\\b"; break;
			case 12: out += "\\f"; break;
			case '\n': out += "\\n"; break;
			case '\r': out += "\\r"; break;
			case '\t': out += "\\t"; break;
			case 11: out += "\\v"; break;
			default: out.push_back((char)c); break;
			}
		}
		size_t at = 0;
		while ((at = out.find("\xC2\x85", at)) != std::string::npos)
		{
			out.replace(at, 2, "\\u0085");
			at += 6;
		}
		at = 0;
		while ((at = out.find("\xE2\x80\xA8", at)) != std::string::npos)
		{
			out.replace(at, 3, "\\u2028");
			at += 6;
		}
		at = 0;
		while ((at = out.find("\xE2\x80\xA9", at)) != std::string::npos)
		{
			out.replace(at, 3, "\\u2029");
			at += 6;
		}
		return out;
	}

	static std::string DotNetG(std::string shortest, int precision)
	{
		for (size_t i = 0; i < shortest.size(); i++)
		{
			if (shortest[i] == 'e')
				shortest[i] = 'E';
		}
		size_t epos = shortest.find('E');
		std::string mant = (epos == std::string::npos) ? shortest : shortest.substr(0, epos);
		int exp = 0;
		if (epos != std::string::npos)
			exp = atoi(shortest.substr(epos + 1).c_str());
		bool neg = false;
		if (!mant.empty() && mant[0] == '-')
		{
			neg = true;
			mant = mant.substr(1);
		}
		size_t dot = mant.find('.');
		std::string intpart = (dot == std::string::npos) ? mant : mant.substr(0, dot);
		std::string fracpart = (dot == std::string::npos) ? "" : mant.substr(dot + 1);
		std::string digits = intpart + fracpart;
		size_t nz = digits.find_first_not_of('0');
		if (nz == std::string::npos)
			return "0";
		int norm = exp + (int)intpart.size() - 1 - (int)nz;
		digits = digits.substr(nz);
		if (norm >= -4 && norm < precision)
		{
			std::string out;
			if (neg)
				out.push_back('-');
			if (norm >= 0)
			{
				if ((int)digits.size() > norm + 1)
				{
					out += digits.substr(0, (size_t)norm + 1);
					out.push_back(',');
					out += digits.substr((size_t)norm + 1);
				}
				else
				{
					out += digits;
					out.append((size_t)(norm + 1 - (int)digits.size()), '0');
				}
			}
			else
			{
				out += "0,";
				out.append((size_t)(-norm - 1), '0');
				out += digits;
			}
			return out;
		}
		std::string out;
		if (neg)
			out.push_back('-');
		out.push_back(digits[0]);
		if (digits.size() > 1)
		{
			out.push_back(',');
			out += digits.substr(1);
		}
		out.push_back('E');
		char exptext[16];
		if (exp >= 0)
			snprintf(exptext, sizeof(exptext), "+%02d", exp);
		else
			snprintf(exptext, sizeof(exptext), "-%02d", -exp);
		out += exptext;
		return out;
	}

	static std::string ToCharsShortest(float value, int precision)
	{
		char buffer[40];
		auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
		if (result.ec != std::errc())
			return "?";
		return DotNetG(std::string(buffer, result.ptr), precision);
	}

	static std::string ToCharsShortest(double value, int precision)
	{
		char buffer[40];
		auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
		if (result.ec != std::errc())
			return "?";
		return DotNetG(std::string(buffer, result.ptr), precision);
	}

	std::string C_Attributes::FormatFloat(float value)
	{
		if (value != value)
			return "не число";
		if (value == std::numeric_limits<float>::infinity())
			return "∞";
		if (value == -std::numeric_limits<float>::infinity())
			return "-∞";
		return ToCharsShortest(value, 9);
	}

	std::string C_Attributes::FormatDouble(double value)
	{
		if (value != value)
			return "не число";
		if (value == std::numeric_limits<double>::infinity())
			return "∞";
		if (value == -std::numeric_limits<double>::infinity())
			return "-∞";
		return ToCharsShortest(value, 17);
	}

	std::string C_Attributes::StripAttributeWord(std::string name)
	{
		size_t at = 0;
		while ((at = name.find("Attribute", at)) != std::string::npos)
			name.erase(at, 9);
		return name;
	}

	u32 C_Attributes::ReadEncodedType(u64& pos, u64 end, bool& ok) const
	{
		ok = false;
		if (pos >= end || !m_metadata.RawHas(pos, 1))
			return 0;
		u8 tag = m_metadata.ReadRawU8(pos);
		pos += 1;
		if (tag == 0x55)
		{
			i32 type_idx = m_metadata.ReadCompressedI32(pos, end, ok);
			if (!ok)
				return 0;
			int typedef_idx = m_binary.TypedefFromType(type_idx);
			if (typedef_idx < 0 || (size_t)typedef_idx >= m_metadata.Typedefs().size())
				return 0;
			return m_binary.BinaryTypeKind(m_metadata.Typedefs()[(size_t)typedef_idx].element);
		}
		ok = true;
		return tag;
	}

	bool C_Attributes::ReadConstant(u32 kind, u64& pos, u64 end, S_BlobValue& out_value) const
	{
		out_value = S_BlobValue();
		out_value.kind = kind;
		bool ok = false;
		switch (kind)
		{
		case 0x02:
			if (pos + 1 > end || !m_metadata.RawHas(pos, 1))
				return false;
			out_value.ival = m_metadata.ReadRawU8(pos);
			pos += 1;
			return true;
		case 0x05:
			if (pos + 1 > end || !m_metadata.RawHas(pos, 1))
				return false;
			out_value.uval = m_metadata.ReadRawU8(pos);
			out_value.ival = (i64)out_value.uval;
			pos += 1;
			return true;
		case 0x04:
			if (pos + 1 > end || !m_metadata.RawHas(pos, 1))
				return false;
			out_value.ival = (i8)m_metadata.ReadRawU8(pos);
			out_value.uval = (u64)out_value.ival;
			pos += 1;
			return true;
		case 0x03:
			if (pos + 2 > end || !m_metadata.RawHas(pos, 2))
				return false;
			out_value.ival = m_metadata.ReadRawU16(pos);
			out_value.uval = (u64)out_value.ival;
			pos += 2;
			return true;
		case 0x07:
			if (pos + 2 > end || !m_metadata.RawHas(pos, 2))
				return false;
			out_value.uval = m_metadata.ReadRawU16(pos);
			out_value.ival = (i64)out_value.uval;
			pos += 2;
			return true;
		case 0x06:
			if (pos + 2 > end || !m_metadata.RawHas(pos, 2))
				return false;
			out_value.ival = m_metadata.ReadRawI32(pos) & 0xFFFF;
			if (out_value.ival & 0x8000)
				out_value.ival -= 0x10000;
			out_value.uval = (u64)out_value.ival;
			pos += 2;
			return true;
		case 0x09:
			out_value.uval = m_metadata.ReadCompressedU32(pos, end, ok);
			if (!ok)
				return false;
			out_value.ival = (i64)out_value.uval;
			return true;
		case 0x08:
			out_value.ival = m_metadata.ReadCompressedI32(pos, end, ok);
			if (!ok)
				return false;
			out_value.uval = (u64)out_value.ival;
			return true;
		case 0x0B:
			if (pos + 8 > end || !m_metadata.RawHas(pos, 8))
				return false;
			out_value.uval = m_metadata.ReadRawU64(pos);
			out_value.ival = (i64)out_value.uval;
			pos += 8;
			return true;
		case 0x0A:
			if (pos + 8 > end || !m_metadata.RawHas(pos, 8))
				return false;
			out_value.ival = m_metadata.ReadRawI64(pos);
			out_value.uval = (u64)out_value.ival;
			pos += 8;
			return true;
		case 0x0C:
			if (pos + 4 > end || !m_metadata.RawHas(pos, 4))
				return false;
			{
				u32 raw = m_metadata.ReadRawU32(pos);
				float value = 0.0f;
				memcpy(&value, &raw, 4);
				out_value.fval = value;
				pos += 4;
				return true;
			}
		case 0x0D:
			if (pos + 8 > end || !m_metadata.RawHas(pos, 8))
				return false;
			{
				u64 raw = m_metadata.ReadRawU64(pos);
				double value = 0.0;
				memcpy(&value, &raw, 8);
				out_value.fval = value;
				pos += 8;
				return true;
			}
		case 0x0E:
		{
			i32 length = m_metadata.ReadCompressedI32(pos, end, ok);
			if (!ok)
				return false;
			if (length == -1)
			{
				out_value.is_null = true;
				return true;
			}
			if (length < 0 || (u64)length > end - pos || !m_metadata.RawHas(pos, (u64)length))
				return false;
			out_value.sval.assign(m_metadata.RawPtr(pos), (size_t)length);
			pos += (u64)length;
			return true;
		}
		case 0x1D:
		{
			i32 count = m_metadata.ReadCompressedI32(pos, end, ok);
			if (!ok)
				return false;
			if (count == -1)
			{
				out_value.is_null = true;
				return true;
			}
			if (count < 0 || count > 4096)
				return false;
			u32 element = ReadEncodedType(pos, end, ok);
			if (!ok)
				return false;
			if (pos + 1 > end || !m_metadata.RawHas(pos, 1))
				return false;
			u8 different = m_metadata.ReadRawU8(pos);
			pos += 1;
			for (i32 i = 0; i < count; i++)
			{
				u32 item_kind = element;
				if (different == 1)
				{
					item_kind = ReadEncodedType(pos, end, ok);
					if (!ok)
						return false;
				}
				S_BlobValue item;
				if (!ReadConstant(item_kind, pos, end, item))
					return false;
				out_value.items.push_back(item);
			}
			return true;
		}
		case 0xFF:
		{
			i32 type_idx = m_metadata.ReadCompressedI32(pos, end, ok);
			if (!ok)
				return false;
			if (type_idx == -1)
			{
				out_value.is_null = true;
				return true;
			}
			out_value.ival = type_idx;
			out_value.uval = (u64)type_idx;
			return true;
		}
		default:
			return false;
		}
	}

	std::string C_Attributes::FormatBlobValue(const S_BlobValue& value) const
	{
		if (value.is_null)
			return "null";
		switch (value.kind)
		{
		case 0x02:
			return value.ival ? "True" : "False";
		case 0x03:
		{
			std::string out;
			AppendChar(out, (unsigned)value.uval);
			return out;
		}
		case 0x04:
		case 0x06:
		case 0x08:
		case 0x0A:
		{
			char text[32];
			snprintf(text, sizeof(text), "%lld", (long long)value.ival);
			return text;
		}
		case 0x05:
		case 0x07:
		case 0x09:
		case 0x0B:
		{
			char text[32];
			snprintf(text, sizeof(text), "%llu", (unsigned long long)value.uval);
			return text;
		}
		case 0x0C:
			return FormatFloat((float)value.fval);
		case 0x0D:
			return FormatDouble(value.fval);
		case 0x0E:
			return "\"" + value.sval + "\"";
		case 0x1D:
		{
			std::string out = "new[] { ";
			for (size_t i = 0; i < value.items.size(); i++)
			{
				if (i > 0)
					out += ", ";
				out += FormatBlobValue(value.items[i]);
			}
			out += " }";
			return out;
		}
		case 0xFF:
			return "typeof(" + m_binary.TypeName((int)value.ival, -1, -1) + ")";
		default:
			return "?";
		}
	}

	bool C_Attributes::DefaultValueText(int type_idx, int data_idx, std::string& out_text, u64& out_fail_ptr) const
	{
		out_text.clear();
		out_fail_ptr = m_metadata.DefaultDataBase() + (u64)(i64)data_idx;
		if (data_idx < 0)
			return false;
		u64 pos = out_fail_ptr;
		u64 end = m_metadata.RawSize();
		u32 kind = m_binary.BinaryTypeKind(type_idx);
		S_BlobValue value;
		if (!ReadConstant(kind, pos, end, value))
			return false;
		if (value.is_null)
		{
			out_text = "null";
			return true;
		}
		if (kind == 0x0E)
		{
			out_text = "\"" + EscapeDumpString(value.sval) + "\"";
			return true;
		}
		if (kind == 0x03)
		{
			char text[16];
			snprintf(text, sizeof(text), "'\\x%x'", (unsigned)value.uval);
			out_text = text;
			return true;
		}
		out_text = FormatBlobValue(value);
		return true;
	}

	std::string C_Attributes::CustomAttributes(int image_index, u32 token, const std::string& indent) const
	{
		u64 blob_off = 0;
		u64 blob_size = 0;
		if (!m_metadata.AttributeBlob(image_index, token, blob_off, blob_size))
			return "";
		u64 end = blob_off + blob_size;
		if (blob_size < 1 || !m_metadata.RawHas(blob_off, blob_size))
			return "";
		u64 pos = blob_off;
		bool ok = false;
		u32 count = m_metadata.ReadCompressedU32(pos, end, ok);
		if (!ok || count > 256)
			return "";
		if (count == 0)
			return "";
		u64 ctor_mark = pos;
		u64 data_mark = pos + (u64)count * 4;
		if (data_mark > end)
			return "";
		const std::vector<S_MethodDef>& methods = m_metadata.Methods();
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		const std::vector<S_FieldDef>& fields = m_metadata.Fields();
		const std::vector<S_PropertyDef>& properties = m_metadata.Properties();
		std::string out;
		for (u32 i = 0; i < count; i++)
		{
			u64 ctor_at = ctor_mark + (u64)i * 4;
			if (ctor_at + 4 > end || !m_metadata.RawHas(ctor_at, 4))
				return out;
			i32 ctor = m_metadata.ReadRawI32(ctor_at);
			if (ctor < 0 || (size_t)ctor >= methods.size())
				continue;
			int declaring = methods[(size_t)ctor].declaring;
			if (declaring < 0 || (size_t)declaring >= typedefs.size())
				continue;
			u64 data_pos = data_mark;
			u32 argc = m_metadata.ReadCompressedU32(data_pos, end, ok);
			if (!ok || argc > 64)
				continue;
			u32 fcnt = m_metadata.ReadCompressedU32(data_pos, end, ok);
			if (!ok || fcnt > 64)
				continue;
			u32 pcnt = m_metadata.ReadCompressedU32(data_pos, end, ok);
			if (!ok || pcnt > 64)
				continue;
			std::vector<std::string> args;
			bool bad = false;
			for (u32 a = 0; a < argc; a++)
			{
				u32 element = ReadEncodedType(data_pos, end, ok);
				if (!ok)
				{
					bad = true;
					break;
				}
				S_BlobValue value;
				if (!ReadConstant(element, data_pos, end, value))
				{
					bad = true;
					break;
				}
				args.push_back(FormatBlobValue(value));
			}
			const S_TypeDef& attr_type = typedefs[(size_t)declaring];
			for (u32 f = 0; f < fcnt && !bad; f++)
			{
				S_BlobValue value;
				u32 element = ReadEncodedType(data_pos, end, ok);
				if (!ok)
				{
					bad = true;
					break;
				}
				if (!ReadConstant(element, data_pos, end, value))
				{
					bad = true;
					break;
				}
				i32 member = m_metadata.ReadCompressedI32(data_pos, end, ok);
				if (!ok)
				{
					bad = true;
					break;
				}
				int field_typedef = declaring;
				int field_index = member;
				if (member < 0)
				{
					field_index = -(member + 1);
					u32 type_ref = m_metadata.ReadCompressedU32(data_pos, end, ok);
					if (!ok)
					{
						bad = true;
						break;
					}
					field_typedef = (int)type_ref;
				}
				std::string field_name = "?";
				if (field_typedef >= 0 && (size_t)field_typedef < typedefs.size())
				{
					int global = typedefs[(size_t)field_typedef].field_start + field_index;
					if (global >= 0 && (size_t)global < fields.size())
						field_name = m_metadata.GetString(fields[(size_t)global].name_idx);
				}
				args.push_back(field_name + " = " + FormatBlobValue(value));
			}
			for (u32 p = 0; p < pcnt && !bad; p++)
			{
				S_BlobValue value;
				u32 element = ReadEncodedType(data_pos, end, ok);
				if (!ok)
				{
					bad = true;
					break;
				}
				if (!ReadConstant(element, data_pos, end, value))
				{
					bad = true;
					break;
				}
				i32 member = m_metadata.ReadCompressedI32(data_pos, end, ok);
				if (!ok)
				{
					bad = true;
					break;
				}
				int prop_typedef = declaring;
				int prop_index = member;
				if (member < 0)
				{
					prop_index = -(member + 1);
					u32 type_ref = m_metadata.ReadCompressedU32(data_pos, end, ok);
					if (!ok)
					{
						bad = true;
						break;
					}
					prop_typedef = (int)type_ref;
				}
				std::string prop_name = "?";
				if (prop_typedef >= 0 && (size_t)prop_typedef < typedefs.size())
				{
					int global = typedefs[(size_t)prop_typedef].property_start + prop_index;
					if (global >= 0 && (size_t)global < properties.size())
						prop_name = m_metadata.GetString(properties[(size_t)global].name_idx);
				}
				args.push_back(prop_name + " = " + FormatBlobValue(value));
			}
			if (bad)
				continue;
			data_mark = data_pos;
			std::string type_name = StripAttributeWord(m_metadata.GetString(attr_type.name_idx));
			out += indent + "[" + type_name;
			if (!args.empty())
			{
				out += "(";
				for (size_t a = 0; a < args.size(); a++)
				{
					if (a > 0)
						out += ", ";
					out += args[a];
				}
				out += ")";
			}
			out += "]\n";
		}
		return out;
	}
}
