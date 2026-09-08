#include "Il2cppBinary.hpp"

namespace AvirA
{
	static const u8 k_MscorlibNeedle[] =
	{
		'm', 's', 'c', 'o', 'r', 'l', 'i', 'b', '.', 'd', 'l', 'l', 0x00
	};

	bool C_Il2cppBinary::Load(const std::string& path)
	{
		m_has_registration = false;
		if (!m_file.LoadFromFile(path))
			return false;
		return ParsePe();
	}

	bool C_Il2cppBinary::HasRegistration() const
	{
		return m_has_registration;
	}

	u64 C_Il2cppBinary::ImageBase() const
	{
		return m_image_base;
	}

	u64 C_Il2cppBinary::FileSize() const
	{
		return m_file.Size();
	}

	const C_Il2cppMetadata* C_Il2cppBinary::Metadata() const
	{
		return m_metadata;
	}

	bool C_Il2cppBinary::ParsePe()
	{
		m_sections.clear();
		if (m_file.Size() < 0x40 || m_file.ReadU16(0) != 0x5A4D)
			return false;
		u32 lfanew = m_file.ReadU32(0x3C);
		if (!m_file.Has(lfanew, 6) || m_file.ReadU32(lfanew) != 0x4550)
			return false;
		u16 sections = m_file.ReadU16(lfanew + 6);
		u16 opt_size = m_file.ReadU16(lfanew + 20);
		size_t opt = (size_t)lfanew + 24;
		if (!m_file.Has(opt, 26) || m_file.ReadU16(opt) != 0x20B)
			return false;
		m_image_base = m_file.ReadU64(opt + 24);
		if (m_image_base == 0)
			return false;
		size_t at = (size_t)lfanew + 24 + opt_size;
		for (u16 i = 0; i < sections; i++)
		{
			if (!m_file.Has(at, 40))
				return false;
			S_PeSection section;
			char name[9] = {};
			for (int k = 0; k < 8; k++)
				name[k] = (char)m_file.Data()[at + k];
			section.name = name;
			u32 vsize = m_file.ReadU32(at + 8);
			u32 vaddr = m_file.ReadU32(at + 12);
			u32 raw_size = m_file.ReadU32(at + 16);
			u32 raw_ptr = m_file.ReadU32(at + 20);
			section.virt_addr = vaddr;
			section.virt_size = vsize > raw_size ? vsize : raw_size;
			section.raw_ptr = raw_ptr;
			section.raw_size = raw_size;
			section.chars = m_file.ReadU32(at + 36);
			m_sections.push_back(section);
			at += 40;
		}
		return !m_sections.empty();
	}

	u64 C_Il2cppBinary::RawFromVa(u64 va) const
	{
		if (va < m_image_base)
			return UINT64_MAX;
		u64 rva = va - m_image_base;
		for (size_t i = 0; i < m_sections.size(); i++)
		{
			const S_PeSection& section = m_sections[i];
			if (rva >= section.virt_addr && rva < section.virt_addr + section.virt_size)
			{
				u64 off = rva - section.virt_addr;
				if (off >= section.raw_size)
					return UINT64_MAX;
				if (section.raw_ptr + off >= m_file.Size())
					return UINT64_MAX;
				return section.raw_ptr + off;
			}
		}
		return UINT64_MAX;
	}

	u64 C_Il2cppBinary::VaFromRaw(u64 raw) const
	{
		for (size_t i = 0; i < m_sections.size(); i++)
		{
			const S_PeSection& section = m_sections[i];
			if (raw >= section.raw_ptr && raw < section.raw_ptr + section.raw_size)
				return m_image_base + section.virt_addr + (raw - section.raw_ptr);
		}
		return 0;
	}

	bool C_Il2cppBinary::BinaryHas(u64 raw, u64 size) const
	{
		return m_file.Has((size_t)raw, (size_t)size);
	}

	u64 C_Il2cppBinary::BinaryReadU64(u64 raw) const
	{
		if (!BinaryHas(raw, 8))
			return 0;
		return m_file.ReadU64((size_t)raw);
	}

	i64 C_Il2cppBinary::BinaryReadI64(u64 raw) const
	{
		return (i64)BinaryReadU64(raw);
	}

	u8 C_Il2cppBinary::BinaryReadU8(u64 raw) const
	{
		if (!BinaryHas(raw, 1))
			return 0;
		return m_file.Data()[(size_t)raw];
	}

	u64 C_Il2cppBinary::ReadVaU64(u64 va) const
	{
		u64 raw = RawFromVa(va);
		if (raw == UINT64_MAX)
			return 0;
		return m_file.ReadU64((size_t)raw);
	}

	u32 C_Il2cppBinary::ReadVaU32(u64 va) const
	{
		u64 raw = RawFromVa(va);
		if (raw == UINT64_MAX)
			return 0;
		return m_file.ReadU32((size_t)raw);
	}

	u8 C_Il2cppBinary::ReadVaU8(u64 va) const
	{
		u64 raw = RawFromVa(va);
		if (raw == UINT64_MAX || raw >= m_file.Size())
			return 0;
		return m_file.Data()[raw];
	}

	bool C_Il2cppBinary::IsDataVa(u64 va) const
	{
		if (va < m_image_base)
			return false;
		u64 rva = va - m_image_base;
		for (size_t i = 0; i < m_sections.size(); i++)
		{
			const S_PeSection& section = m_sections[i];
			if (section.chars != 0x40000040 && section.chars != 0xC0000040)
				continue;
			if (rva >= section.virt_addr && rva < section.virt_addr + section.virt_size)
				return true;
		}
		return false;
	}

	bool C_Il2cppBinary::IsExecVa(u64 va) const
	{
		if (va < m_image_base)
			return false;
		u64 rva = va - m_image_base;
		for (size_t i = 0; i < m_sections.size(); i++)
		{
			const S_PeSection& section = m_sections[i];
			if (section.chars != 0x60000020)
				continue;
			if (rva >= section.virt_addr && rva < section.virt_addr + section.virt_size)
				return true;
		}
		return false;
	}

	std::vector<u64> C_Il2cppBinary::FindRefs(u64 target_va, bool in_data) const
	{
		std::vector<u64> out;
		u8 needle[8];
		for (int i = 0; i < 8; i++)
			needle[i] = (u8)(target_va >> (i * 8));
		for (size_t i = 0; i < m_sections.size(); i++)
		{
			const S_PeSection& section = m_sections[i];
			bool want = in_data
				? (section.chars == 0x40000040 || section.chars == 0xC0000040)
				: (section.chars == 0x60000020);
			if (!want || section.raw_size < 8)
				continue;
			if (section.raw_ptr + section.raw_size > m_file.Size())
				continue;
			const u8* base = m_file.Data() + section.raw_ptr;
			for (u64 k = 0; k + 8 <= section.raw_size; k += 8)
			{
				bool hit = true;
				for (int b = 0; b < 8; b++)
				{
					if (base[k + b] != needle[b])
					{
						hit = false;
						break;
					}
				}
				if (hit)
					out.push_back(m_image_base + section.virt_addr + k);
			}
		}
		return out;
	}

	bool C_Il2cppBinary::FindCodeRegistration(const C_Il2cppMetadata& metadata, u64& out_code_reg)
	{
		out_code_reg = 0;
		int version = metadata.Version() * 10;
		if (version < 242)
			return false;
		size_t image_count = metadata.Images().size();
		if (image_count == 0)
			return false;

		std::vector<u64> dll_vas;
		for (size_t i = 0; i < m_sections.size(); i++)
		{
			const S_PeSection& section = m_sections[i];
			if (section.chars != 0x40000040 && section.chars != 0xC0000040)
				continue;
			if (section.raw_ptr + section.raw_size > m_file.Size())
				continue;
			const u8* base = m_file.Data() + section.raw_ptr;
			for (u64 k = 0; k + sizeof(k_MscorlibNeedle) <= section.raw_size; k++)
			{
				bool hit = true;
				for (size_t b = 0; b < sizeof(k_MscorlibNeedle); b++)
				{
					if (base[k + b] != k_MscorlibNeedle[b])
					{
						hit = false;
						break;
					}
				}
				if (hit)
					dll_vas.push_back(m_image_base + section.virt_addr + k);
			}
		}
		if (dll_vas.empty())
			return false;

		for (size_t d = 0; d < dll_vas.size(); d++)
		{
			std::vector<u64> refs1 = FindRefs(dll_vas[d], true);
			for (size_t a = 0; a < refs1.size(); a++)
			{
				std::vector<u64> refs2 = FindRefs(refs1[a], true);
				for (size_t b = 0; b < refs2.size(); b++)
				{
					for (size_t img = image_count; img-- > 0;)
					{
						std::vector<u64> refs3 = FindRefs(refs2[b] - img * 8, true);
						for (size_t c = 0; c < refs3.size(); c++)
						{
							if (ReadVaU64(refs3[c] - 8) == (u64)image_count)
							{
								out_code_reg = refs3[c] - 14 * 8;
								return true;
							}
						}
					}
				}
			}
		}
		return false;
	}

	static bool SaneCount(u64 value, u64 limit)
	{
		return value > 0 && value < limit;
	}

	bool C_Il2cppBinary::ReadCodeRegistration(u64 code_reg_va, const C_Il2cppMetadata& metadata)
	{
		u64 try_vas[2] = { code_reg_va - 16, code_reg_va };
		size_t image_count = metadata.Images().size();
		for (int t = 0; t < 2; t++)
		{
			u64 base = try_vas[t];
			if (!IsDataVa(base))
				continue;
			u64 gmpc = ReadVaU64(base + 16);
			u64 gmp = ReadVaU64(base + 24);
			u64 ipc = ReadVaU64(base + 40);
			u64 cgmc = ReadVaU64(base + 120);
			u64 cgm = ReadVaU64(base + 128);
			if (!SaneCount(gmpc, 0x50000) || !IsDataVa(gmp))
				continue;
			if (cgmc != (u64)image_count || !IsDataVa(cgm))
				continue;
			u64 first_module = ReadVaU64(cgm);
			if (!IsDataVa(first_module))
				continue;
			u64 name_ptr = ReadVaU64(first_module);
			u64 name_raw = RawFromVa(name_ptr);
			if (name_raw == UINT64_MAX)
				continue;
			bool has_dll = false;
			for (int k = 0; k < 64; k++)
			{
				if (name_raw + (u64)k + 4 > m_file.Size())
					break;
				if (m_file.Data()[name_raw + k] == '.' && m_file.Data()[name_raw + k + 1] == 'd'
					&& m_file.Data()[name_raw + k + 2] == 'l' && m_file.Data()[name_raw + k + 3] == 'l')
				{
					has_dll = true;
					break;
				}
				if (m_file.Data()[name_raw + k] == 0)
					break;
			}
			if (!has_dll)
				continue;

			m_modules.clear();
			for (size_t i = 0; i < image_count; i++)
			{
				u64 module_va = ReadVaU64(cgm + i * 8);
				if (!IsDataVa(module_va))
					break;
				S_CodeGenModule module;
				u64 raw = RawFromVa(ReadVaU64(module_va));
				if (raw == UINT64_MAX)
					break;
				size_t k = 0;
				while (k < 128 && raw + k < m_file.Size() && m_file.Data()[raw + k] != 0)
				{
					module.name.push_back((char)m_file.Data()[raw + k]);
					k++;
				}
				if (module.name.empty())
					break;
				module.method_pointer_count = ReadVaU64(module_va + 8);
				module.method_pointers = ReadVaU64(module_va + 16);
				m_modules.push_back(module);
			}
			if (m_modules.empty())
				continue;

			u64 invoker_count = ipc;
			u64 invoker_ptr = ReadVaU64(base + 48);
			if (SaneCount(invoker_count, 0x100000) && IsDataVa(invoker_ptr))
			{
				m_invoker_pointers.clear();
				for (u64 i = 0; i < invoker_count; i++)
					m_invoker_pointers.push_back(ReadVaU64(invoker_ptr + i * 8));
			}
			u64 generic_ptr = gmp;
			m_generic_method_pointers.clear();
			for (u64 i = 0; i < gmpc; i++)
				m_generic_method_pointers.push_back(ReadVaU64(generic_ptr + i * 8));
			return true;
		}
		m_modules.clear();
		return false;
	}

	bool C_Il2cppBinary::FindMetadataRegistration(const C_Il2cppMetadata& metadata, u64& out_meta_reg)
	{
		out_meta_reg = 0;
		if (metadata.Version() * 10 < 190)
			return false;
		size_t typedef_count = metadata.Typedefs().size();
		if (typedef_count == 0)
			return false;
		for (size_t i = 0; i < m_sections.size(); i++)
		{
			const S_PeSection& section = m_sections[i];
			if (section.chars != 0x40000040 && section.chars != 0xC0000040)
				continue;
			if (section.raw_ptr + section.raw_size > m_file.Size() || section.raw_size < 32)
				continue;
			u64 rows = section.raw_size / 8;
			for (u64 k = 0; k + 3 < rows; k++)
			{
				u64 v0 = m_file.ReadU64((size_t)(section.raw_ptr + k * 8));
				if (v0 != (u64)typedef_count)
					continue;
				u64 v2 = m_file.ReadU64((size_t)(section.raw_ptr + (k + 2) * 8));
				if (v2 != (u64)typedef_count)
					continue;
				u64 ptr_va = m_file.ReadU64((size_t)(section.raw_ptr + (k + 1) * 8));
				u64 ptr_raw = RawFromVa(ptr_va);
				if (ptr_raw == UINT64_MAX)
					continue;
				bool all_data = true;
				u64 check = typedef_count > 64 ? 64 : (u64)typedef_count;
				for (u64 j = 0; j < check; j++)
				{
					u64 entry = m_file.ReadU64((size_t)(ptr_raw + j * 8));
					if (entry != 0 && !IsDataVa(entry))
					{
						all_data = false;
						break;
					}
				}
				if (!all_data)
					continue;
				out_meta_reg = m_image_base + section.virt_addr + k * 8 - 10 * 8;
				return true;
			}
		}
		return false;
	}

	bool C_Il2cppBinary::ReadMetadataRegistration(u64 meta_reg_va, const C_Il2cppMetadata& metadata)
	{
		if (!IsDataVa(meta_reg_va))
			return false;
		size_t typedef_count = metadata.Typedefs().size();
		u64 types_count = ReadVaU64(meta_reg_va + 6 * 8);
		u64 types_ptr = ReadVaU64(meta_reg_va + 7 * 8);
		u64 specs_count = ReadVaU64(meta_reg_va + 8 * 8);
		u64 specs_ptr = ReadVaU64(meta_reg_va + 9 * 8);
		u64 offsets_count = ReadVaU64(meta_reg_va + 10 * 8);
		u64 offsets_ptr = ReadVaU64(meta_reg_va + 11 * 8);
		u64 table_count = ReadVaU64(meta_reg_va + 4 * 8);
		u64 table_ptr = ReadVaU64(meta_reg_va + 5 * 8);
		u64 insts_count = ReadVaU64(meta_reg_va + 2 * 8);
		u64 insts_ptr = ReadVaU64(meta_reg_va + 3 * 8);
		if (offsets_count != (u64)typedef_count || !IsDataVa(offsets_ptr))
			return false;
		if (types_count == 0 || types_count > 4000000 || !IsDataVa(types_ptr))
			return false;

		int version = metadata.Version() * 10;
		m_types.clear();
		m_type_ptrs.clear();
		m_type_ptr_map.clear();
		for (u64 i = 0; i < types_count; i++)
		{
			u64 type_va = ReadVaU64(types_ptr + i * 8);
			u64 type_raw = RawFromVa(type_va);
			S_Il2CppType type{};
			if (type_raw != UINT64_MAX && type_raw + 16 <= m_file.Size())
			{
				type.datapoint = m_file.ReadU64((size_t)type_raw);
				u32 bits = m_file.ReadU32((size_t)type_raw + 8);
				type.attrs = bits & 0xFFFF;
				type.kind = (bits >> 16) & 0xFF;
				if (version >= 272)
				{
					type.num_mods = (bits >> 24) & 0x1F;
					type.byref = (bits >> 29) & 1;
				}
				else
				{
					type.num_mods = (bits >> 24) & 0x3F;
					type.byref = (bits >> 30) & 1;
				}
			}
			m_types.push_back(type);
			m_type_ptrs.push_back(type_va);
			m_type_ptr_map[type_va] = (int)m_types.size() - 1;
		}

		m_field_offset_ptrs.clear();
		for (size_t i = 0; i < typedef_count; i++)
			m_field_offset_ptrs.push_back(ReadVaU64(offsets_ptr + i * 8));

		m_method_specs.clear();
		if (specs_count > 0 && specs_count < 4000000 && IsDataVa(specs_ptr))
		{
			for (u64 i = 0; i < specs_count; i++)
			{
				S_MethodSpecRaw spec;
				spec.method_def = -1;
				spec.class_inst = -1;
				spec.method_inst = -1;
				u64 raw = RawFromVa(specs_ptr + i * 12);
				if (raw != UINT64_MAX && raw + 12 <= m_file.Size())
				{
					spec.method_def = m_file.ReadI32((size_t)raw);
					spec.class_inst = m_file.ReadI32((size_t)raw + 4);
					spec.method_inst = m_file.ReadI32((size_t)raw + 8);
				}
				m_method_specs.push_back(spec);
			}
		}

		m_generic_methods.clear();
		size_t entry_size = version >= 271 ? 16 : 12;
		if (table_count > 0 && table_count < 4000000 && IsDataVa(table_ptr))
		{
			for (u64 i = 0; i < table_count; i++)
			{
				S_GenericMethodRaw entry;
				entry.spec_index = -1;
				entry.method_index = -1;
				entry.invoker_index = -1;
				u64 raw = RawFromVa(table_ptr + i * entry_size);
				if (raw != UINT64_MAX && raw + 12 <= m_file.Size())
				{
					entry.spec_index = m_file.ReadI32((size_t)raw);
					entry.method_index = m_file.ReadI32((size_t)raw + 4);
					entry.invoker_index = m_file.ReadI32((size_t)raw + 8);
				}
				m_generic_methods.push_back(entry);
			}
		}

		m_generic_insts.clear();
		if (insts_count > 0 && insts_count < 4000000 && IsDataVa(insts_ptr))
		{
			for (u64 i = 0; i < insts_count; i++)
				m_generic_insts.push_back(ReadVaU64(insts_ptr + i * 8));
		}
		return true;
	}

	bool C_Il2cppBinary::Resolve(const C_Il2cppMetadata& metadata)
	{
		m_metadata = &metadata;
		m_binary_version = metadata.Version() * 10;
		m_has_registration = false;
		if (m_binary_version < 242)
			return false;
		u64 code_reg = 0;
		u64 meta_reg = 0;
		if (!FindCodeRegistration(metadata, code_reg))
		{
			C_Log::Fail("FindCodeRegistration failed");
			return false;
		}
		C_Log::Info("CodeRegistration candidate set");
		if (!ReadCodeRegistration(code_reg, metadata))
		{
			C_Log::Fail("ReadCodeRegistration failed");
			return false;
		}
		if (!FindMetadataRegistration(metadata, meta_reg))
		{
			C_Log::Fail("FindMetadataRegistration failed");
			return false;
		}
		C_Log::Info("MetadataRegistration candidate set");
		if (!ReadMetadataRegistration(meta_reg, metadata))
		{
			C_Log::Fail("ReadMetadataRegistration failed");
			return false;
		}
		m_has_registration = true;
		return true;
	}

	u64 C_Il2cppBinary::MethodRva(const std::string& image_name, u32 token) const
	{
		if (!m_has_registration || token == 0)
			return UINT64_MAX;
		for (size_t i = 0; i < m_modules.size(); i++)
		{
			if (m_modules[i].name != image_name)
				continue;
			u64 index = (u64)(token & 0x00FFFFFFu);
			if (index == 0 || index > m_modules[i].method_pointer_count)
				return UINT64_MAX;
			u64 va = ReadVaU64(m_modules[i].method_pointers + (index - 1) * 8);
			if (va == 0 || (!IsExecVa(va) && !IsDataVa(va)))
				return UINT64_MAX;
			return va - m_image_base;
		}
		return UINT64_MAX;
	}

	u64 C_Il2cppBinary::FileOffsetFromRva(u64 rva) const
	{
		if (rva == UINT64_MAX)
			return UINT64_MAX;
		return RawFromVa(m_image_base + rva);
	}

	i32 C_Il2cppBinary::FieldOffset(int type_index, int field_index_in_type, bool is_valuetype, bool is_static) const
	{
		if (!m_has_registration || type_index < 0 || field_index_in_type < 0)
			return -1;
		if ((size_t)type_index >= m_field_offset_ptrs.size())
			return -1;
		u64 table = m_field_offset_ptrs[(size_t)type_index];
		if (table == 0)
			return -1;
		u64 raw = RawFromVa(table + (u64)field_index_in_type * 4);
		if (raw == UINT64_MAX || raw + 4 > m_file.Size())
			return -1;
		i32 offset = m_file.ReadI32((size_t)raw);
		if (offset > 0 && is_valuetype && !is_static)
			offset -= 16;
		return offset;
	}

	int C_Il2cppBinary::TypeIndexFromPtr(u64 type_ptr) const
	{
		auto it = m_type_ptr_map.find(type_ptr);
		if (it == m_type_ptr_map.end())
			return -1;
		return it->second;
	}

	int C_Il2cppBinary::FindTypeByPtr(u64 type_ptr) const
	{
		return TypeIndexFromPtr(type_ptr);
	}

	bool C_Il2cppBinary::BinaryTypeRaw(int type_index, u64& datapoint, u32& kind) const
	{
		if (type_index < 0 || (size_t)type_index >= m_types.size())
			return false;
		datapoint = m_types[(size_t)type_index].datapoint;
		kind = m_types[(size_t)type_index].kind;
		return true;
	}

	u32 C_Il2cppBinary::BinaryTypeKind(int type_index) const
	{
		if (type_index < 0 || (size_t)type_index >= m_types.size())
			return 0x08;
		return m_types[(size_t)type_index].kind;
	}

	u32 C_Il2cppBinary::BinaryTypeAttrs(int type_index) const
	{
		if (type_index < 0 || (size_t)type_index >= m_types.size())
			return 0;
		return m_types[(size_t)type_index].attrs;
	}

	u32 C_Il2cppBinary::BinaryTypeByref(int type_index) const
	{
		if (type_index < 0 || (size_t)type_index >= m_types.size())
			return 0;
		if (m_types[(size_t)type_index].kind == 0x10)
			return 1;
		return m_types[(size_t)type_index].byref;
	}

	int C_Il2cppBinary::TypedefFromType(int type_index) const
	{
		if (type_index < 0 || (size_t)type_index >= m_types.size() || !m_metadata)
			return -1;
		const S_Il2CppType& type = m_types[(size_t)type_index];
		if (type.kind == 0x11 || type.kind == 0x12 || type.kind == 0x55)
		{
			i64 klass = (i64)type.datapoint;
			if (klass < 0 || (size_t)klass >= m_metadata->Typedefs().size())
				return -1;
			return (int)klass;
		}
		if (type.kind == 0x15)
		{
			u64 raw = RawFromVa(type.datapoint);
			if (raw == UINT64_MAX || raw + 8 > m_file.Size())
				return -1;
			return TypedefFromType(TypeIndexFromPtr(m_file.ReadU64((size_t)raw)));
		}
		return -1;
	}

	static std::string StripArityName(const std::string& name)
	{
		size_t tick = name.find('`');
		if (tick == std::string::npos)
			return name;
		return name.substr(0, tick);
	}

	std::string C_Il2cppBinary::NestedDisplayName(int typedef_index, bool for_declaration) const
	{
		if (!m_metadata || typedef_index < 0 || (size_t)typedef_index >= m_metadata->Typedefs().size())
			return "?";
		const std::vector<S_TypeDef>& typedefs = m_metadata->Typedefs();
		std::string name = m_metadata->GetString(typedefs[(size_t)typedef_index].name_idx);
		bool strip = !for_declaration || typedefs[(size_t)typedef_index].generic_container >= 0;
		if (strip)
			name = StripArityName(name);
		int declaring = typedefs[(size_t)typedef_index].declaring;
		while (declaring >= 0)
		{
			u64 datapoint = 0;
			u32 kind = 0;
			if (!BinaryTypeRaw(declaring, datapoint, kind))
				break;
			if (kind == 0x11 || kind == 0x12 || kind == 0x55 || kind == 0x15)
			{
				int parent = TypedefFromType(declaring);
				if (parent < 0 || (size_t)parent >= typedefs.size())
					break;
				name = StripArityName(m_metadata->GetString(typedefs[(size_t)parent].name_idx)) + "." + name;
				declaring = typedefs[(size_t)parent].declaring;
				continue;
			}
			name = TypeName(declaring, -1, -1) + "." + name;
			break;
		}
		return name;
	}

	std::string C_Il2cppBinary::RawNestedName(int typedef_index) const
	{
		if (!m_metadata || typedef_index < 0 || (size_t)typedef_index >= m_metadata->Typedefs().size())
			return "?";
		const std::vector<S_TypeDef>& typedefs = m_metadata->Typedefs();
		std::string name = m_metadata->GetString(typedefs[(size_t)typedef_index].name_idx);
		int declaring = typedefs[(size_t)typedef_index].declaring;
		while (declaring >= 0)
		{
			u64 datapoint = 0;
			u32 kind = 0;
			if (!BinaryTypeRaw(declaring, datapoint, kind))
				break;
			int parent = -1;
			if (kind == 0x11 || kind == 0x12 || kind == 0x55 || kind == 0x15)
				parent = TypedefFromType(declaring);
			if (parent < 0 || (size_t)parent >= typedefs.size())
				break;
			name = m_metadata->GetString(typedefs[(size_t)parent].name_idx) + "." + name;
			declaring = typedefs[(size_t)parent].declaring;
		}
		return name;
	}

	std::string C_Il2cppBinary::TypedefDeclName(int typedef_index) const
	{
		std::string name = NestedDisplayName(typedef_index, true);
		if (!m_metadata || typedef_index < 0 || (size_t)typedef_index >= m_metadata->Typedefs().size())
			return name;
		int container = m_metadata->Typedefs()[(size_t)typedef_index].generic_container;
		if (container >= 0 && (size_t)container < m_metadata->GenericContainers().size())
		{
			const S_GenericContainer& gc = m_metadata->GenericContainers()[(size_t)container];
			const std::vector<S_GenericParam>& gparams = m_metadata->GenericParams();
			name.push_back('<');
			for (int i = 0; i < gc.argc; i++)
			{
				if (i > 0)
					name += ", ";
				size_t global = (size_t)gc.param_start + i;
				if (global < gparams.size())
					name += m_metadata->GetString(gparams[global].name_idx);
			}
			name.push_back('>');
		}
		return name;
	}

	std::string C_Il2cppBinary::GenericParamName(int container_idx, int num, bool is_method) const
	{
		if (!m_metadata || container_idx < 0 || (size_t)container_idx >= m_metadata->GenericContainers().size())
			return is_method ? "TMethod" : "T";
		const std::vector<S_GenericParam>& params = m_metadata->GenericParams();
		int seen = 0;
		for (size_t i = 0; i < params.size(); i++)
		{
			if (params[i].owner != container_idx)
				continue;
			if (seen == num)
				return m_metadata->GetString(params[i].name_idx);
			seen++;
		}
		return is_method ? "TMethod" : "T";
	}

	static const char* PrimitiveName(u32 kind)
	{
		switch (kind)
		{
		case 0x01: return "void";
		case 0x02: return "bool";
		case 0x03: return "char";
		case 0x04: return "sbyte";
		case 0x05: return "byte";
		case 0x06: return "short";
		case 0x07: return "ushort";
		case 0x08: return "int";
		case 0x09: return "uint";
		case 0x0A: return "long";
		case 0x0B: return "ulong";
		case 0x0C: return "float";
		case 0x0D: return "double";
		case 0x0E: return "string";
		case 0x18: return "IntPtr";
		case 0x19: return "UIntPtr";
		case 0x1C: return "object";
		case 0x16: return "TypedReference";
		default: return nullptr;
		}
	}

	std::string C_Il2cppBinary::TypeName(int type_index, int context_typedef, int context_method, int depth) const
	{
		if (!m_metadata || !m_has_registration)
			return "?";
		if (depth > 16)
			return "?";
		if (type_index < 0 || (size_t)type_index >= m_types.size())
			return "?";
		const S_Il2CppType& type = m_types[(size_t)type_index];
		if (const char* primitive = PrimitiveName(type.kind))
			return primitive;
		const std::vector<S_TypeDef>& typedefs = m_metadata->Typedefs();
		const std::vector<S_MethodDef>& methods = m_metadata->Methods();

		switch (type.kind)
		{
		case 0x0F:
		{
			int inner = TypeIndexFromPtr(type.datapoint);
			return TypeName(inner, context_typedef, context_method, depth + 1) + "*";
		}
		case 0x10:
		{
			int inner = TypeIndexFromPtr(type.datapoint);
			return TypeName(inner, context_typedef, context_method, depth + 1) + "&";
		}
		case 0x11:
		case 0x12:
		case 0x55:
		{
			i64 klass = (i64)type.datapoint;
			if (klass < 0 || (size_t)klass >= typedefs.size())
				return "?";
			std::string name = NestedDisplayName((int)klass, false);
			int container = typedefs[(size_t)klass].generic_container;
			if (container >= 0 && (size_t)container < m_metadata->GenericContainers().size())
			{
				const S_GenericContainer& gc = m_metadata->GenericContainers()[(size_t)container];
				const std::vector<S_GenericParam>& gparams = m_metadata->GenericParams();
				name.push_back('<');
				for (int i = 0; i < gc.argc; i++)
				{
					if (i > 0)
						name += ", ";
					size_t global = (size_t)gc.param_start + i;
					if (global < gparams.size())
						name += m_metadata->GetString(gparams[global].name_idx);
				}
				name.push_back('>');
			}
			return name;
		}
		case 0x13:
		case 0x1E:
		{
			const std::vector<S_GenericParam>& params = m_metadata->GenericParams();
			if (type.datapoint < (u64)params.size())
			{
				std::string pname = m_metadata->GetString(params[(size_t)type.datapoint].name_idx);
				if (!pname.empty())
					return pname;
			}
			int container = -1;
			bool is_method = (type.kind == 0x1E);
			if (!is_method && context_typedef >= 0 && (size_t)context_typedef < typedefs.size())
				container = typedefs[(size_t)context_typedef].generic_container;
			if (is_method && context_method >= 0 && (size_t)context_method < methods.size())
				container = methods[(size_t)context_method].generic_container;
			return GenericParamName(container, (int)type.datapoint, is_method);
		}
		case 0x1D:
		{
			int inner = TypeIndexFromPtr(type.datapoint);
			return TypeName(inner, context_typedef, context_method, depth + 1) + "[]";
		}
		case 0x14:
		{
			u64 raw = RawFromVa(type.datapoint);
			int inner = -1;
			int rank = 1;
			if (raw != UINT64_MAX && raw + 16 <= m_file.Size())
			{
				u64 etype = m_file.ReadU64((size_t)raw);
				inner = TypeIndexFromPtr(etype);
				rank = m_file.Data()[raw + 8];
				if (rank <= 0)
					rank = 1;
			}
			std::string name = TypeName(inner, context_typedef, context_method, depth + 1);
			name.push_back('[');
			for (int i = 1; i < rank; i++)
				name.push_back(',');
			name.push_back(']');
			return name;
		}
		case 0x15:
		{
			u64 raw = RawFromVa(type.datapoint);
			if (raw == UINT64_MAX || raw + 24 > m_file.Size())
				return "?";
			u64 generic_type_ptr = m_file.ReadU64((size_t)raw);
			u64 class_inst_ptr = m_file.ReadU64((size_t)raw + 8);
			int generic_type = TypeIndexFromPtr(generic_type_ptr);
			int generic_typedef = TypedefFromType(generic_type);
			std::string name = (generic_typedef >= 0)
				? NestedDisplayName(generic_typedef, false)
				: TypeName(generic_type, context_typedef, context_method, depth + 1);
			u64 inst_raw = RawFromVa(class_inst_ptr);
			if (inst_raw == UINT64_MAX || inst_raw + 16 > m_file.Size())
				return name;
			i64 argc = m_file.ReadI64((size_t)inst_raw);
			u64 argv = m_file.ReadU64((size_t)inst_raw + 8);
			if (argc <= 0 || argc > 16)
				return name;
			name.push_back('<');
			for (i64 i = 0; i < argc; i++)
			{
				if (i > 0)
					name += ", ";
				u64 arg_ptr = ReadVaU64(argv + (u64)i * 8);
				name += TypeName(TypeIndexFromPtr(arg_ptr), context_typedef, context_method, depth + 1);
			}
			name.push_back('>');
			return name;
		}
		case 0xFF:
			return TypeName((int)type.datapoint, context_typedef, context_method, depth + 1);
		default:
			return "?";
		}
	}

	std::string C_Il2cppBinary::GenericInstArgs(u64 inst_va, int context_method) const
	{
		u64 inst_raw = RawFromVa(inst_va);
		if (inst_raw == UINT64_MAX || inst_raw + 16 > m_file.Size())
			return "";
		i64 argc = m_file.ReadI64((size_t)inst_raw);
		u64 argv = m_file.ReadU64((size_t)inst_raw + 8);
		if (argc <= 0 || argc > 16)
			return "";
		std::string out = "<";
		for (i64 k = 0; k < argc; k++)
		{
			if (k > 0)
				out += ", ";
			out += TypeName(TypeIndexFromPtr(ReadVaU64(argv + (u64)k * 8)), -1, context_method);
		}
		out += ">";
		return out;
	}

	std::vector<S_GenericInstance> C_Il2cppBinary::GenericInstancesFull(int method_def_index) const
	{
		std::vector<S_GenericInstance> out;
		if (!m_metadata || !m_has_registration || method_def_index < 0)
			return out;
		if ((size_t)method_def_index >= m_metadata->Methods().size())
			return out;
		const S_MethodDef& method = m_metadata->Methods()[(size_t)method_def_index];
		bool strip_declaring = false;
		if (method.declaring >= 0 && (size_t)method.declaring < m_metadata->Typedefs().size())
			strip_declaring = m_metadata->Typedefs()[(size_t)method.declaring].generic_container >= 0;
		for (size_t i = 0; i < m_generic_methods.size(); i++)
		{
			const S_GenericMethodRaw& entry = m_generic_methods[i];
			if (entry.spec_index < 0 || (size_t)entry.spec_index >= m_method_specs.size())
				continue;
			const S_MethodSpecRaw& spec = m_method_specs[(size_t)entry.spec_index];
			if (spec.method_def != method_def_index)
				continue;
			S_GenericInstance instance;
			instance.pointer = 0;
			if (entry.method_index >= 0 && (size_t)entry.method_index < m_generic_method_pointers.size())
				instance.pointer = m_generic_method_pointers[(size_t)entry.method_index];
			if (instance.pointer != 0)
			{
				instance.rva = instance.pointer - m_image_base;
				u64 raw = RawFromVa(instance.pointer);
				instance.file_offset = (raw == UINT64_MAX) ? 0 : raw;
			}
			std::string type_name_text = NestedDisplayName(method.declaring, strip_declaring);
			if (spec.class_inst >= 0 && (size_t)spec.class_inst < m_generic_insts.size())
				type_name_text += GenericInstArgs(m_generic_insts[(size_t)spec.class_inst], method_def_index);
			std::string method_name_text = m_metadata->GetString(method.name_idx);
			if (spec.method_inst >= 0 && (size_t)spec.method_inst < m_generic_insts.size())
				method_name_text += GenericInstArgs(m_generic_insts[(size_t)spec.method_inst], method_def_index);
			instance.display_name = type_name_text + "." + method_name_text;
			instance.method_name = method_name_text;
			out.push_back(instance);
		}
		return out;
	}
}
