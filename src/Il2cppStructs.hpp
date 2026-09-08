#pragma once
#include "AvirA.hpp"

namespace AvirA
{
	struct S_FieldGate
	{
		u8 size;
		int min_v;
		int max_v;
	};

	inline bool GateOpen(const S_FieldGate& gate, int version)
	{
		return version >= gate.min_v && version <= gate.max_v;
	}

	inline size_t GatedStructSize(const S_FieldGate* fields, int count, int version)
	{
		size_t size = 0;
		for (int i = 0; i < count; i++)
		{
			if (GateOpen(fields[i], version))
				size += fields[i].size;
		}
		return size;
	}

	class C_StructCursor
	{
	public:
		const u8* base;
		size_t left;
		bool ok;

		C_StructCursor(const u8* base_, size_t size_)
			: base(base_), left(size_), ok(base_ != nullptr)
		{
		}

		u64 Take(const S_FieldGate& gate, int version, u64 fallback = 0)
		{
			if (!GateOpen(gate, version))
				return fallback;
			if (!ok || left < gate.size)
			{
				ok = false;
				return fallback;
			}
			u64 value = 0;
			for (u8 i = 0; i < gate.size; i++)
				value |= (u64)base[i] << (i * 8);
			base += gate.size;
			left -= gate.size;
			return value;
		}

		i64 TakeSigned(const S_FieldGate& gate, int version)
		{
			u64 raw = Take(gate, version, 0);
			if (gate.size == 4)
				return (i64)(i32)(u32)raw;
			if (gate.size == 2)
				return (i64)(i16)(u16)raw;
			return (i64)raw;
		}
	};

	struct S_ImageDef
	{
		u32 name_idx = 0;
		i32 asm_idx = 0;
		i32 type_start = 0;
		u32 type_count = 0;
		i32 entry_point = 0;
		u32 token = 0;
		i32 custom_attr_start = 0;
		u32 custom_attr_count = 0;
	};

	struct S_AssemblyName
	{
		u32 name_idx = 0;
		u32 culture_idx = 0;
		u32 pubkey_idx = 0;
		u32 hash_alg = 0;
		i32 hash_len = 0;
		u32 flags = 0;
		i32 major = 0;
		i32 minor = 0;
		i32 build = 0;
		i32 revision = 0;
		u8 pubkey_token[8] = {};
	};

	struct S_AssemblyDef
	{
		i32 image_idx = 0;
		u32 token = 0;
		i32 ref_start = 0;
		i32 ref_count = 0;
		S_AssemblyName aname;
	};

	struct S_TypeDef
	{
		u32 name_idx = 0;
		u32 ns_idx = 0;
		i32 byval_idx = 0;
		i32 byref_idx = 0;
		i32 declaring = -1;
		i32 parent = -1;
		i32 element = -1;
		i32 generic_container = -1;
		u32 flags = 0;
		i32 field_start = 0;
		i32 method_start = 0;
		i32 event_start = 0;
		i32 property_start = 0;
		i32 nested_start = 0;
		i32 interfaces_start = 0;
		i32 vtable_start = 0;
		i32 iface_offsets_start = 0;
		u16 method_count = 0;
		u16 property_count = 0;
		u16 field_count = 0;
		u16 event_count = 0;
		u16 nested_count = 0;
		u16 vtable_count = 0;
		u16 interfaces_count = 0;
		u16 iface_offsets_count = 0;
		u32 bitfield = 0;
		u32 token = 0;
	};

	struct S_MethodDef
	{
		u32 name_idx = 0;
		i32 declaring = -1;
		i32 ret_idx = 0;
		i32 param_start = 0;
		i32 generic_container = -1;
		u32 token = 0;
		u16 flags = 0;
		u16 iflags = 0;
		u16 slot = 0;
		u16 param_count = 0;
	};

	struct S_ParamDef
	{
		u32 name_idx = 0;
		u32 token = 0;
		i32 type_idx = 0;
	};

	struct S_FieldDef
	{
		u32 name_idx = 0;
		u32 token = 0;
		i32 type_idx = 0;
	};

	struct S_PropertyDef
	{
		u32 name_idx = 0;
		i32 getter = -1;
		i32 setter = -1;
		u32 attrs = 0;
		u32 token = 0;
	};

	struct S_EventDef
	{
		u32 name_idx = 0;
		i32 type_idx = 0;
		i32 adder = -1;
		i32 remover = -1;
		i32 raiser = -1;
		u32 token = 0;
	};

	struct S_GenericContainer
	{
		i32 owner = -1;
		i32 argc = 0;
		i32 is_method = 0;
		i32 param_start = 0;
	};

	struct S_GenericParam
	{
		i32 owner = -1;
		u32 name_idx = 0;
		u16 constr_start = 0;
		u16 constr_count = 0;
		u16 num = 0;
		u16 flags = 0;
	};

	struct S_StringLiteral
	{
		u32 length = 0;
		i32 data_index = 0;
	};

	struct S_DefaultValue
	{
		i32 index = 0;
		i32 type_idx = 0;
		i32 data_index = 0;
	};

	struct S_AttributeDataRange
	{
		u32 token = 0;
		u32 start_offset = 0;
	};

	struct S_MetadataHeader
	{
		u32 sanity = 0;
		i32 version = 0;
		u32 string_literal_offset = 0;
		i32 string_literal_size = 0;
		u32 string_literal_data_offset = 0;
		i32 string_literal_data_size = 0;
		u32 string_offset = 0;
		i32 string_size = 0;
		u32 events_offset = 0;
		i32 events_size = 0;
		u32 properties_offset = 0;
		i32 properties_size = 0;
		u32 methods_offset = 0;
		i32 methods_size = 0;
		u32 param_default_values_offset = 0;
		i32 param_default_values_size = 0;
		u32 field_default_values_offset = 0;
		i32 field_default_values_size = 0;
		u32 field_param_default_data_offset = 0;
		i32 field_param_default_data_size = 0;
		i32 field_marshaled_sizes_offset = 0;
		i32 field_marshaled_sizes_size = 0;
		u32 params_offset = 0;
		i32 params_size = 0;
		u32 fields_offset = 0;
		i32 fields_size = 0;
		u32 generic_params_offset = 0;
		i32 generic_params_size = 0;
		u32 generic_param_constraints_offset = 0;
		i32 generic_param_constraints_size = 0;
		u32 generic_containers_offset = 0;
		i32 generic_containers_size = 0;
		u32 nested_types_offset = 0;
		i32 nested_types_size = 0;
		u32 interfaces_offset = 0;
		i32 interfaces_size = 0;
		u32 vtable_methods_offset = 0;
		i32 vtable_methods_size = 0;
		i32 iface_offsets_offset = 0;
		i32 iface_offsets_size = 0;
		u32 typedefs_offset = 0;
		i32 typedefs_size = 0;
		u32 images_offset = 0;
		i32 images_size = 0;
		u32 assemblies_offset = 0;
		i32 assemblies_size = 0;
		u32 field_refs_offset = 0;
		i32 field_refs_size = 0;
		i32 referenced_assemblies_offset = 0;
		i32 referenced_assemblies_size = 0;
		u32 attribute_data_offset = 0;
		i32 attribute_data_size = 0;
		u32 attribute_data_range_offset = 0;
		i32 attribute_data_range_size = 0;
		i32 unresolved_virtual_call_param_types_offset = 0;
		i32 unresolved_virtual_call_param_types_size = 0;
		i32 unresolved_virtual_call_param_ranges_offset = 0;
		i32 unresolved_virtual_call_param_ranges_size = 0;
		i32 windows_runtime_type_names_offset = 0;
		i32 windows_runtime_type_names_size = 0;
		i32 windows_runtime_strings_offset = 0;
		i32 windows_runtime_strings_size = 0;
		i32 exported_typedefs_offset = 0;
		i32 exported_typedefs_size = 0;
	};
}
