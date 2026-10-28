#include "Il2cppMetadata.hpp"

namespace AvirA
{
	bool C_Il2cppMetadata::Load(const std::string& path)
	{
		m_version = 0;
		return m_file.LoadFromFile(path);
	}

	bool C_Il2cppMetadata::PatchVersionByte()
	{
		if (m_file.Size() < 8)
			return false;
		if (m_file.Data()[4] == AVIRA_METADATA_PATCH_FROM)
		{
			m_file.Data()[4] = AVIRA_METADATA_PATCH_TO;
			return true;
		}
		return m_file.Data()[4] == AVIRA_METADATA_PATCH_TO;
	}

	int C_Il2cppMetadata::Version() const
	{
		return m_version;
	}

	u64 C_Il2cppMetadata::LiteralDataBase() const
	{
		return m_literal_data_off;
	}

	std::string C_Il2cppMetadata::GetString(u32 index) const
	{
		auto cached = m_string_cache.find(index);
		if (cached != m_string_cache.end())
			return cached->second;
		u64 at = m_strings_off + index;
		if (at >= m_file.Size())
			return "";
		std::string out;
		while (at < m_file.Size() && m_file.Data()[at] != 0)
		{
			out.push_back((char)m_file.Data()[at]);
			at++;
		}
		m_string_cache[index] = out;
		return out;
	}

	bool C_Il2cppMetadata::RawHas(u64 offset, u64 size) const
	{
		return m_file.Has((size_t)offset, (size_t)size);
	}

	u8 C_Il2cppMetadata::ReadRawU8(u64 offset) const
	{
		if (!RawHas(offset, 1))
			return 0;
		return m_file.Data()[offset];
	}

	u16 C_Il2cppMetadata::ReadRawU16(u64 offset) const
	{
		if (!RawHas(offset, 2))
			return 0;
		return m_file.ReadU16((size_t)offset);
	}

	u32 C_Il2cppMetadata::ReadRawU32(u64 offset) const
	{
		if (!RawHas(offset, 4))
			return 0;
		return m_file.ReadU32((size_t)offset);
	}

	u64 C_Il2cppMetadata::ReadRawU64(u64 offset) const
	{
		if (!RawHas(offset, 8))
			return 0;
		return m_file.ReadU64((size_t)offset);
	}

	i32 C_Il2cppMetadata::ReadRawI32(u64 offset) const
	{
		return (i32)ReadRawU32(offset);
	}

	i64 C_Il2cppMetadata::ReadRawI64(u64 offset) const
	{
		return (i64)ReadRawU64(offset);
	}

	const char* C_Il2cppMetadata::RawPtr(u64 offset) const
	{
		if (offset >= m_file.Size())
			return "";
		return (const char*)(m_file.Data() + offset);
	}

	u64 C_Il2cppMetadata::RawSize() const
	{
		return m_file.Size();
	}

	u32 C_Il2cppMetadata::ReadCompressedU32(u64& pos, u64 end, bool& ok) const
	{
		ok = false;
		if (pos >= end || !RawHas(pos, 1))
			return 0;
		u8 first = ReadRawU8(pos);
		pos += 1;
		if ((first & 0x80) == 0)
		{
			ok = true;
			return first;
		}
		if ((first & 0xC0) == 0x80)
		{
			if (pos >= end || !RawHas(pos, 1))
				return 0;
			u8 second = ReadRawU8(pos);
			pos += 1;
			ok = true;
			return ((u32)(first & ~0x80u) << 8) | second;
		}
		if ((first & 0xE0) == 0xC0)
		{
			if (pos + 3 > end || !RawHas(pos, 3))
				return 0;
			u8 b1 = ReadRawU8(pos);
			u8 b2 = ReadRawU8(pos + 1);
			u8 b3 = ReadRawU8(pos + 2);
			pos += 3;
			ok = true;
			return ((u32)(first & ~0xC0u) << 24) | ((u32)b1 << 16) | ((u32)b2 << 8) | b3;
		}
		if (first == 0xF0)
		{
			if (pos + 4 > end || !RawHas(pos, 4))
				return 0;
			u32 value = ReadRawU32(pos);
			pos += 4;
			ok = true;
			return value;
		}
		if (first == 0xFE)
		{
			ok = true;
			return UINT32_MAX - 1;
		}
		if (first == 0xFF)
		{
			ok = true;
			return UINT32_MAX;
		}
		return 0;
	}

	i32 C_Il2cppMetadata::ReadCompressedI32(u64& pos, u64 end, bool& ok) const
	{
		u32 encoded = ReadCompressedU32(pos, end, ok);
		if (!ok)
			return 0;
		if (encoded == UINT32_MAX)
			return INT32_MIN;
		bool negative = (encoded & 1) != 0;
		encoded >>= 1;
		if (negative)
			return -(i32)(encoded + 1);
		return (i32)encoded;
	}

	bool C_Il2cppMetadata::AttributeBlob(int image_index, u32 token, u64& out_offset, u64& out_size) const
	{
		out_offset = 0;
		out_size = 0;
		if (image_index < 0 || (size_t)image_index >= m_image_attr_maps.size())
			return false;
		auto it = m_image_attr_maps[(size_t)image_index].find(token);
		if (it == m_image_attr_maps[(size_t)image_index].end())
			return false;
		size_t range = (size_t)it->second;
		if (range + 1 >= m_attr_ranges.size())
			return false;
		out_offset = m_attr_data_off + m_attr_ranges[range].start;
		u64 end = m_attr_data_off + m_attr_ranges[range + 1].start;
		if (end < out_offset)
			return false;
		out_size = end - out_offset;
		return true;
	}

	u64 C_Il2cppMetadata::AttributeDataBase() const
	{
		return m_attr_data_off;
	}

	bool C_Il2cppMetadata::FieldDefault(int global_field, S_DefaultValue& out_value) const
	{
		auto it = m_field_defaults.find(global_field);
		if (it == m_field_defaults.end())
			return false;
		out_value = it->second;
		return true;
	}

	bool C_Il2cppMetadata::ParamDefault(int global_param, S_DefaultValue& out_value) const
	{
		auto it = m_param_defaults.find(global_param);
		if (it == m_param_defaults.end())
			return false;
		out_value = it->second;
		return true;
	}

	u64 C_Il2cppMetadata::DefaultDataBase() const
	{
		return m_default_data_off;
	}

	const std::vector<S_ImageDef>& C_Il2cppMetadata::Images() const
	{
		return m_images;
	}

	const std::vector<S_TypeDef>& C_Il2cppMetadata::Typedefs() const
	{
		return m_typedefs;
	}

	const std::vector<S_MethodDef>& C_Il2cppMetadata::Methods() const
	{
		return m_methods;
	}

	const std::vector<S_ParamDef>& C_Il2cppMetadata::Params() const
	{
		return m_params;
	}

	const std::vector<S_FieldDef>& C_Il2cppMetadata::Fields() const
	{
		return m_fields;
	}

	const std::vector<S_PropertyDef>& C_Il2cppMetadata::Properties() const
	{
		return m_properties;
	}

	const std::vector<S_GenericContainer>& C_Il2cppMetadata::GenericContainers() const
	{
		return m_containers;
	}

	const std::vector<S_GenericParam>& C_Il2cppMetadata::GenericParams() const
	{
		return m_generic_params;
	}

	const std::vector<S_StringLiteral>& C_Il2cppMetadata::StringLiterals() const
	{
		return m_literals;
	}

	const std::vector<S_FieldRef>& C_Il2cppMetadata::FieldRefs() const
	{
		return m_field_refs;
	}

	const std::vector<int>& C_Il2cppMetadata::InterfaceIndices() const
	{
		return m_interfaces;
	}

	bool C_Il2cppMetadata::ParseHeader()
	{
		if (m_file.Size() < 220 || m_file.ReadU32(0) != 0xFAB11BAF)
			return false;
		m_version = m_file.ReadI32(4);
		if (m_version < 16 || m_version > 1000)
			return false;
		m_strings_off = m_file.ReadU32(24);
		m_literal_data_off = m_file.ReadU32(16);
		m_default_data_off = m_file.ReadU32(72);
		m_attr_data_off = m_file.ReadU32(200);
		return true;
	}

	bool C_Il2cppMetadata::ParseTables()
	{
		if (m_version < 16 || m_version > 1000)
			return false;

		u64 events_off = m_file.ReadU32(32);
		i32 events_size = m_file.ReadI32(36);
		u64 props_off = m_file.ReadU32(40);
		i32 props_size = m_file.ReadI32(44);
		u64 methods_off = m_file.ReadU32(48);
		i32 methods_size = m_file.ReadI32(52);
		u64 param_defs_off = m_file.ReadU32(56);
		i32 param_defs_size = m_file.ReadI32(60);
		u64 field_defs_off = m_file.ReadU32(64);
		i32 field_defs_size = m_file.ReadI32(68);
		u64 params_off = m_file.ReadU32(88);
		i32 params_size = m_file.ReadI32(92);
		u64 fields_off = m_file.ReadU32(96);
		i32 fields_size = m_file.ReadI32(100);
		u64 generic_params_off = m_file.ReadU32(104);
		i32 generic_params_size = m_file.ReadI32(108);
		u64 generic_containers_off = m_file.ReadU32(120);
		i32 generic_containers_size = m_file.ReadI32(124);
		u64 interfaces_off = m_file.ReadU32(136);
		i32 interfaces_size = m_file.ReadI32(140);
		u64 typedefs_off = m_file.ReadU32(160);
		i32 typedefs_size = m_file.ReadI32(164);
		u64 images_off = m_file.ReadU32(168);
		i32 images_size = m_file.ReadI32(172);
		u64 attr_ranges_off = m_file.ReadU32(208);
		i32 attr_ranges_size = m_file.ReadI32(212);
		u64 field_refs_off = m_file.ReadU32(184);
		i32 field_refs_size = m_file.ReadI32(188);

		m_typedefs.clear();
		size_t typedef_count = (size_t)(typedefs_size / 88);
		for (size_t i = 0; i < typedef_count; i++)
		{
			u64 at = typedefs_off + i * 88;
			if (!m_file.Has((size_t)at, 88))
				break;
			S_TypeDef type;
			type.name_idx = m_file.ReadU32((size_t)at);
			type.namespace_idx = m_file.ReadU32((size_t)at + 4);
			type.byval = m_file.ReadI32((size_t)at + 8);
			type.declaring = m_file.ReadI32((size_t)at + 12);
			type.parent = m_file.ReadI32((size_t)at + 16);
			type.element = m_file.ReadI32((size_t)at + 20);
			type.generic_container = m_file.ReadI32((size_t)at + 24);
			type.flags = m_file.ReadU32((size_t)at + 28);
			type.field_start = m_file.ReadI32((size_t)at + 32);
			type.method_start = m_file.ReadI32((size_t)at + 36);
			type.event_start = m_file.ReadI32((size_t)at + 40);
			type.property_start = m_file.ReadI32((size_t)at + 44);
			type.nested_start = m_file.ReadI32((size_t)at + 48);
			type.interfaces_start = m_file.ReadI32((size_t)at + 52);
			type.method_count = m_file.ReadU16((size_t)at + 64);
			type.property_count = m_file.ReadU16((size_t)at + 66);
			type.field_count = m_file.ReadU16((size_t)at + 68);
			type.event_count = m_file.ReadU16((size_t)at + 70);
			type.nested_count = m_file.ReadU16((size_t)at + 72);
			type.interfaces_count = m_file.ReadU16((size_t)at + 76);
			u32 bitfield = m_file.ReadU32((size_t)at + 80);
			type.is_valuetype = (bitfield & 1) != 0;
			type.is_enum = ((bitfield >> 1) & 1) != 0;
			type.token = m_file.ReadU32((size_t)at + 84);
			m_typedefs.push_back(type);
		}

		m_methods.clear();
		size_t method_count = (size_t)(methods_size / 36);
		for (size_t i = 0; i < method_count; i++)
		{
			u64 at = methods_off + i * 36;
			if (!m_file.Has((size_t)at, 36))
				break;
			S_MethodDef method;
			method.name_idx = m_file.ReadU32((size_t)at);
			method.declaring = m_file.ReadI32((size_t)at + 4);
			method.return_type = m_file.ReadI32((size_t)at + 8);
			method.param_start = m_file.ReadI32((size_t)at + 16);
			method.generic_container = m_file.ReadI32((size_t)at + 20);
			method.token = m_file.ReadU32((size_t)at + 24);
			method.flags = m_file.ReadU16((size_t)at + 28);
			method.slot = m_file.ReadU16((size_t)at + 32);
			method.param_count = m_file.ReadU16((size_t)at + 34);
			m_methods.push_back(method);
		}

		m_params.clear();
		size_t param_count = (size_t)(params_size / 12);
		for (size_t i = 0; i < param_count; i++)
		{
			u64 at = params_off + i * 12;
			if (!m_file.Has((size_t)at, 12))
				break;
			S_ParamDef param;
			param.name_idx = m_file.ReadU32((size_t)at);
			param.token = m_file.ReadU32((size_t)at + 4);
			param.type_idx = m_file.ReadI32((size_t)at + 8);
			m_params.push_back(param);
		}

		m_fields.clear();
		size_t field_count = (size_t)(fields_size / 12);
		for (size_t i = 0; i < field_count; i++)
		{
			u64 at = fields_off + i * 12;
			if (!m_file.Has((size_t)at, 12))
				break;
			S_FieldDef field;
			field.name_idx = m_file.ReadU32((size_t)at);
			field.type_idx = m_file.ReadI32((size_t)at + 4);
			field.token = m_file.ReadU32((size_t)at + 8);
			m_fields.push_back(field);
		}

		m_properties.clear();
		size_t property_count = (size_t)(props_size / 20);
		for (size_t i = 0; i < property_count; i++)
		{
			u64 at = props_off + i * 20;
			if (!m_file.Has((size_t)at, 20))
				break;
			S_PropertyDef prop;
			prop.name_idx = m_file.ReadU32((size_t)at);
			prop.get = m_file.ReadI32((size_t)at + 4);
			prop.set = m_file.ReadI32((size_t)at + 8);
			prop.token = m_file.ReadU32((size_t)at + 16);
			m_properties.push_back(prop);
		}

		m_containers.clear();
		size_t container_count = (size_t)(generic_containers_size / 16);
		for (size_t i = 0; i < container_count; i++)
		{
			u64 at = generic_containers_off + i * 16;
			if (!m_file.Has((size_t)at, 16))
				break;
			S_GenericContainer container;
			container.owner = m_file.ReadI32((size_t)at);
			container.argc = m_file.ReadI32((size_t)at + 4);
			container.is_method = m_file.ReadI32((size_t)at + 8);
			container.param_start = m_file.ReadI32((size_t)at + 12);
			m_containers.push_back(container);
		}

		m_generic_params.clear();
		size_t generic_param_count = (size_t)(generic_params_size / 16);
		for (size_t i = 0; i < generic_param_count; i++)
		{
			u64 at = generic_params_off + i * 16;
			if (!m_file.Has((size_t)at, 16))
				break;
			S_GenericParam param;
			param.owner = m_file.ReadI32((size_t)at);
			param.name_idx = m_file.ReadU32((size_t)at + 4);
			param.num = m_file.ReadU16((size_t)at + 12);
			m_generic_params.push_back(param);
		}

		m_interfaces.clear();
		size_t interface_count = (size_t)(interfaces_size / 4);
		for (size_t i = 0; i < interface_count; i++)
		{
			u64 at = interfaces_off + i * 4;
			if (!m_file.Has((size_t)at, 4))
				break;
			m_interfaces.push_back(m_file.ReadI32((size_t)at));
		}

		m_field_refs.clear();
		size_t field_ref_count = (size_t)(field_refs_size / 8);
		for (size_t i = 0; i < field_ref_count; i++)
		{
			u64 at = field_refs_off + i * 8;
			if (!m_file.Has((size_t)at, 8))
				break;
			S_FieldRef ref;
			ref.type_idx = m_file.ReadI32((size_t)at);
			ref.field_idx = m_file.ReadI32((size_t)at + 4);
			m_field_refs.push_back(ref);
		}

		m_literals.clear();
		u64 literal_off = m_file.ReadU32(8);
		size_t literal_count = (size_t)(m_file.ReadU32(12) / 8);
		for (size_t i = 0; i < literal_count; i++)
		{
			u64 at = literal_off + i * 8;
			if (!m_file.Has((size_t)at, 8))
				break;
			S_StringLiteral literal;
			literal.length = m_file.ReadU32((size_t)at);
			literal.offset = m_file.ReadI32((size_t)at + 4);
			m_literals.push_back(literal);
		}

		m_images.clear();
		size_t image_count = (size_t)(images_size / 40);
		for (size_t i = 0; i < image_count; i++)
		{
			u64 at = images_off + i * 40;
			if (!m_file.Has((size_t)at, 40))
				break;
			S_ImageDef image;
			image.name = GetString(m_file.ReadU32((size_t)at));
			image.start = m_file.ReadI32((size_t)at + 8);
			image.count = m_file.ReadU32((size_t)at + 12);
			image.attr_start = m_file.ReadI32((size_t)at + 32);
			image.attr_count = m_file.ReadU32((size_t)at + 36);
			m_images.push_back(image);
		}

		m_attr_ranges.clear();
		size_t range_count = (size_t)(attr_ranges_size / 8);
		for (size_t i = 0; i < range_count; i++)
		{
			u64 at = attr_ranges_off + i * 8;
			if (!m_file.Has((size_t)at, 8))
				break;
			S_AttributeDataRange range;
			range.token = m_file.ReadU32((size_t)at);
			range.start = m_file.ReadU32((size_t)at + 4);
			m_attr_ranges.push_back(range);
		}

		m_image_attr_maps.clear();
		m_image_attr_maps.resize(m_images.size());
		for (size_t i = 0; i < m_images.size(); i++)
		{
			for (u32 k = 0; k < m_images[i].attr_count; k++)
			{
				size_t range = (size_t)m_images[i].attr_start + k;
				if (range >= m_attr_ranges.size())
					break;
				m_image_attr_maps[i][m_attr_ranges[range].token] = (int)range;
			}
		}

		m_field_defaults.clear();
		size_t field_default_count = (size_t)(field_defs_size / 12);
		for (size_t i = 0; i < field_default_count; i++)
		{
			u64 at = field_defs_off + i * 12;
			if (!m_file.Has((size_t)at, 12))
				break;
			S_DefaultValue value;
			value.type_idx = m_file.ReadI32((size_t)at + 4);
			value.data_idx = m_file.ReadI32((size_t)at + 8);
			m_field_defaults[m_file.ReadI32((size_t)at)] = value;
		}

		m_param_defaults.clear();
		size_t param_default_count = (size_t)(param_defs_size / 12);
		for (size_t i = 0; i < param_default_count; i++)
		{
			u64 at = param_defs_off + i * 12;
			if (!m_file.Has((size_t)at, 12))
				break;
			S_DefaultValue value;
			value.type_idx = m_file.ReadI32((size_t)at + 4);
			value.data_idx = m_file.ReadI32((size_t)at + 8);
			m_param_defaults[m_file.ReadI32((size_t)at)] = value;
		}

		(void)events_off;
		(void)events_size;
		return !m_typedefs.empty() && !m_images.empty();
	}
}
