#pragma once
#include "AvirA.hpp"

namespace AvirA
{
	struct S_ImageDef
	{
		std::string name;
		int start = 0;
		u32 count = 0;
		int attr_start = 0;
		u32 attr_count = 0;
	};

	struct S_TypeDef
	{
		u32 name_idx = 0;
		u32 namespace_idx = 0;
		int byval = -1;
		int declaring = -1;
		int parent = -1;
		int element = -1;
		int generic_container = -1;
		u32 flags = 0;
		int field_start = 0;
		int method_start = 0;
		int event_start = 0;
		int property_start = 0;
		int nested_start = 0;
		int interfaces_start = 0;
		int method_count = 0;
		int property_count = 0;
		int field_count = 0;
		int event_count = 0;
		int nested_count = 0;
		int interfaces_count = 0;
		bool is_valuetype = false;
		bool is_enum = false;
		u32 token = 0;
	};

	struct S_MethodDef
	{
		u32 name_idx = 0;
		int declaring = -1;
		int return_type = -1;
		int param_start = 0;
		int generic_container = -1;
		u32 token = 0;
		u32 flags = 0;
		u32 slot = 0;
		int param_count = 0;
	};

	struct S_ParamDef
	{
		u32 name_idx = 0;
		u32 token = 0;
		int type_idx = -1;
	};

	struct S_FieldDef
	{
		u32 name_idx = 0;
		int type_idx = -1;
		u32 token = 0;
	};

	struct S_PropertyDef
	{
		u32 name_idx = 0;
		int get = -1;
		int set = -1;
		u32 token = 0;
	};

	struct S_GenericContainer
	{
		int owner = -1;
		int argc = 0;
		int is_method = 0;
		int param_start = 0;
	};

	struct S_GenericParam
	{
		int owner = -1;
		u32 name_idx = 0;
		int num = 0;
	};

	struct S_StringLiteral
	{
		u32 length = 0;
		i32 offset = 0;
	};

	struct S_FieldRef
	{
		int type_idx = -1;
		int field_idx = -1;
	};

	struct S_AttributeDataRange
	{
		u32 token = 0;
		u32 start = 0;
	};

	struct S_DefaultValue
	{
		int type_idx = -1;
		int data_idx = -1;
	};

	class C_Il2cppMetadata
	{
	public:
		bool Load(const std::string& path);
		bool PatchVersionByte();
		bool ParseHeader();
		bool ParseTables();

		int Version() const;
		std::string GetString(u32 index) const;
		u64 LiteralDataBase() const;

		bool RawHas(u64 offset, u64 size) const;
		u8 ReadRawU8(u64 offset) const;
		u16 ReadRawU16(u64 offset) const;
		u32 ReadRawU32(u64 offset) const;
		u64 ReadRawU64(u64 offset) const;
		i32 ReadRawI32(u64 offset) const;
		i64 ReadRawI64(u64 offset) const;
		const char* RawPtr(u64 offset) const;
		u64 RawSize() const;

		u32 ReadCompressedU32(u64& pos, u64 end, bool& ok) const;
		i32 ReadCompressedI32(u64& pos, u64 end, bool& ok) const;

		bool AttributeBlob(int image_index, u32 token, u64& out_offset, u64& out_size) const;
		u64 AttributeDataBase() const;

		bool FieldDefault(int global_field, S_DefaultValue& out_value) const;
		bool ParamDefault(int global_param, S_DefaultValue& out_value) const;
		u64 DefaultDataBase() const;

		const std::vector<S_ImageDef>& Images() const;
		const std::vector<S_TypeDef>& Typedefs() const;
		const std::vector<S_MethodDef>& Methods() const;
		const std::vector<S_ParamDef>& Params() const;
		const std::vector<S_FieldDef>& Fields() const;
		const std::vector<S_PropertyDef>& Properties() const;
		const std::vector<S_GenericContainer>& GenericContainers() const;
		const std::vector<S_GenericParam>& GenericParams() const;
		const std::vector<S_StringLiteral>& StringLiterals() const;
		const std::vector<S_FieldRef>& FieldRefs() const;
		const std::vector<int>& InterfaceIndices() const;

	private:
		C_Buffer m_file;
		int m_version = 0;

		u64 m_strings_off = 0;
		u64 m_literal_data_off = 0;
		u64 m_default_data_off = 0;
		u64 m_attr_data_off = 0;

		std::vector<S_ImageDef> m_images;
		std::vector<S_TypeDef> m_typedefs;
		std::vector<S_MethodDef> m_methods;
		std::vector<S_ParamDef> m_params;
		std::vector<S_FieldDef> m_fields;
		std::vector<S_PropertyDef> m_properties;
		std::vector<S_GenericContainer> m_containers;
		std::vector<S_GenericParam> m_generic_params;
		std::vector<S_StringLiteral> m_literals;
		std::vector<S_FieldRef> m_field_refs;
		std::vector<int> m_interfaces;
		std::vector<S_AttributeDataRange> m_attr_ranges;
		std::vector<std::unordered_map<u32, int>> m_image_attr_maps;
		std::unordered_map<int, S_DefaultValue> m_field_defaults;
		std::unordered_map<int, S_DefaultValue> m_param_defaults;
		mutable std::unordered_map<u32, std::string> m_string_cache;
	};
}
