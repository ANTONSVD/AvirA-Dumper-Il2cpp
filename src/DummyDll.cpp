#include "DummyDll.hpp"
#include <Windows.h>

namespace AvirA
{
	struct S_Builder
	{
		std::vector<u8> bytes;

		void PutU8(u8 value)
		{
			bytes.push_back(value);
		}

		void PutU16(u16 value)
		{
			bytes.push_back((u8)(value & 0xFF));
			bytes.push_back((u8)((value >> 8) & 0xFF));
		}

		void PutU32(u32 value)
		{
			bytes.push_back((u8)(value & 0xFF));
			bytes.push_back((u8)((value >> 8) & 0xFF));
			bytes.push_back((u8)((value >> 16) & 0xFF));
			bytes.push_back((u8)((value >> 24) & 0xFF));
		}

		void PutU64(u64 value)
		{
			PutU32((u32)(value & 0xFFFFFFFFu));
			PutU32((u32)((value >> 32) & 0xFFFFFFFFu));
		}

		void PutCompressed(u32 value)
		{
			if (value < 0x80)
				PutU8((u8)value);
			else if (value < 0x4000)
			{
				PutU8((u8)(((value >> 8) & 0x3F) | 0x80));
				PutU8((u8)(value & 0xFF));
			}
			else
			{
				PutU8((u8)(((value >> 24) & 0x1F) | 0xC0));
				PutU8((u8)((value >> 16) & 0xFF));
				PutU8((u8)((value >> 8) & 0xFF));
				PutU8((u8)(value & 0xFF));
			}
		}

		void PutCoded(u32 value)
		{
			PutCompressed(value);
		}

		void PutData(const void* data, size_t size)
		{
			const u8* at = (const u8*)data;
			for (size_t i = 0; i < size; i++)
				bytes.push_back(at[i]);
		}

		void PutStr(u32 value)
		{
			PutU32(value);
		}

		void PutBlobRef(u32 value)
		{
			PutU32(value);
		}

		void PutBuilder(const S_Builder& other)
		{
			for (size_t i = 0; i < other.bytes.size(); i++)
				bytes.push_back(other.bytes[i]);
		}

		void PutString(const std::string& text)
		{
			for (size_t i = 0; i < text.size(); i++)
				bytes.push_back((u8)text[i]);
		}

		size_t Size() const
		{
			return bytes.size();
		}
	};

	struct S_StringHeap
	{
		S_Builder data;
		std::unordered_map<std::string, u32> map;

		S_StringHeap()
		{
			data.PutU8(0);
		}

		u32 Add(const std::string& text)
		{
			auto it = map.find(text);
			if (it != map.end())
				return it->second;
			u32 offset = (u32)data.Size();
			data.PutString(text);
			data.PutU8(0);
			map[text] = offset;
			return offset;
		}
	};

	struct S_BlobHeap
	{
		S_Builder data;

		S_BlobHeap()
		{
			data.PutU8(0);
		}

		u32 Add(const S_Builder& blob)
		{
			u32 offset = (u32)data.Size();
			data.PutCompressed((u32)blob.Size());
			data.PutBuilder(blob);
			return offset;
		}
	};

	struct S_UserStringHeap
	{
		S_Builder data;

		S_UserStringHeap()
		{
			data.PutU8(0);
		}
	};

	class C_DummyAssembly
	{
	public:
		C_DummyAssembly(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary, const std::vector<int>& typedef_image)
			: m_metadata(metadata), m_binary(binary), m_typedef_image(typedef_image)
		{
		}

		void SetWidths(int rid, int has_const, int type_or_method, int typedef_or_ref)
		{
			m_w_rid = rid;
			m_w_has_const = has_const;
			m_w_type_or_method = type_or_method;
			m_w_typedef_or_ref = typedef_or_ref;
		}

		void PutRid(S_Builder& out, u32 value)
		{
			if (m_w_rid == 4)
				out.PutU32(value);
			else
				out.PutU16((u16)value);
		}

		void PutCoded(S_Builder& out, u32 value, int width)
		{
			if (width == 4)
				out.PutU32(value);
			else
				out.PutU16((u16)value);
		}

		std::vector<u8> Build(int image_index)
		{
			m_image = image_index;
			m_typedef_rids.clear();
			m_typeref_map.clear();
			m_assemblyref_map.clear();
			m_typespec_map.clear();
			m_strings = S_StringHeap();
			m_blob = S_BlobHeap();
			m_us = S_UserStringHeap();
			m_module = S_Builder();
			m_typeref = S_Builder();
			m_typedef = S_Builder();
			m_field = S_Builder();
			m_method = S_Builder();
			m_param = S_Builder();
			m_constant = S_Builder();
			m_nested = S_Builder();
			m_typespec = S_Builder();
			m_genericparam = S_Builder();
			m_assembly = S_Builder();
			m_assemblyref = S_Builder();
			m_typedef_count = 0;
			m_typeref_count = 0;
			m_field_count = 0;
			m_method_count = 0;
			m_param_count = 0;
			m_constant_count = 0;
			m_nested_count = 0;
			m_typespec_count = 0;
			m_genericparam_count = 0;
			m_assemblyref_count = 0;

			const std::vector<S_ImageDef>& images = m_metadata.Images();
			const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
			const S_ImageDef& image = images[(size_t)image_index];
			std::string stem = image.name;
			if (stem.size() > 4 && stem.substr(stem.size() - 4) == ".dll")
				stem = stem.substr(0, stem.size() - 4);

			int type_end = image.start + (int)image.count;
			m_typedef_rids.assign(typedefs.size(), 0);
			for (int t = image.start; t < type_end; t++)
			{
				if (t < 0 || (size_t)t >= typedefs.size())
					break;
				m_typedef_rids[(size_t)t] = ++m_typedef_count;
			}

			m_module.PutU16(0);
			m_module.PutStr(m_strings.Add(image.name));
			m_module.PutU16(1);
			m_module.PutU16(1);
			m_module.PutU16(1);

			for (int t = image.start; t < type_end; t++)
			{
				if (t < 0 || (size_t)t >= typedefs.size())
					break;
				WriteTypedefRow(t);
			}

			m_assembly.PutU32(0);
			m_assembly.PutU16(0);
			m_assembly.PutU16(0);
			m_assembly.PutU16(0);
			m_assembly.PutU16(0);
			m_assembly.PutU32(0);
			m_assembly.PutBlobRef(0);
			m_assembly.PutStr(m_strings.Add(stem));
			m_assembly.PutStr(0);

			return WritePe(image.name);
		}

	private:
		int TypedefImage(int typedef_index) const
		{
			if (typedef_index < 0 || (size_t)typedef_index >= m_typedef_image.size())
				return -1;
			return m_typedef_image[(size_t)typedef_index];
		}

		u32 TypeDefOrRef(int type_idx)
		{
			int typedef_idx = m_binary.TypedefFromType(type_idx);
			if (typedef_idx < 0)
				return ((u32)ObjectRef() << 2) | 1;
			int other_image = TypedefImage(typedef_idx);
			if (other_image == m_image)
				return ((u32)m_typedef_rids[(size_t)typedef_idx] << 2) | 0;
			const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
			const std::vector<S_ImageDef>& images = m_metadata.Images();
			std::string stem = images[(size_t)other_image].name;
			if (stem.size() > 4 && stem.substr(stem.size() - 4) == ".dll")
				stem = stem.substr(0, stem.size() - 4);
			u32 ref = FindTypeRef(stem, m_metadata.GetString(typedefs[(size_t)typedef_idx].namespace_idx),
				m_metadata.GetString(typedefs[(size_t)typedef_idx].name_idx));
			return (ref << 2) | 1;
		}

		u32 FindTypeRef(const std::string& assembly, const std::string& ns, const std::string& name)
		{
			std::string key = assembly + '\0' + ns + '\0' + name;
			auto it = m_typeref_map.find(key);
			if (it != m_typeref_map.end())
				return it->second;
			u32 assembly_ref = FindAssemblyRef(assembly);
			u32 rid = ++m_typeref_count;
			PutCoded(m_typeref, (assembly_ref << 2) | 2, 2);
			m_typeref.PutStr(m_strings.Add(name));
			m_typeref.PutStr(m_strings.Add(ns));
			m_typeref_map[key] = rid;
			return rid;
		}

		u32 ObjectRef()
		{
			return FindTypeRef("mscorlib", "System", "Object");
		}

		u32 FindAssemblyRef(const std::string& assembly)
		{
			auto it = m_assemblyref_map.find(assembly);
			if (it != m_assemblyref_map.end())
				return it->second;
			u32 rid = ++m_assemblyref_count;
			m_assemblyref.PutU16(0);
			m_assemblyref.PutU16(0);
			m_assemblyref.PutU16(0);
			m_assemblyref.PutU16(0);
			m_assemblyref.PutU32(0);
			m_assemblyref.PutBlobRef(0);
			m_assemblyref.PutStr(m_strings.Add(assembly));
			m_assemblyref.PutStr(0);
			m_assemblyref.PutBlobRef(0);
			m_assemblyref_map[assembly] = rid;
			return rid;
		}

		void EncodeType(int type_idx, S_Builder& sig, int depth)
		{
			if (depth > 12)
			{
				sig.PutU8(0x1C);
				return;
			}
			u64 datapoint = 0;
			u32 kind = m_binary.BinaryTypeKind(type_idx);
			m_binary.BinaryTypeRaw(type_idx, datapoint, kind);
			switch (kind)
			{
			case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06:
			case 0x07: case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C:
			case 0x0D: case 0x0E: case 0x16: case 0x18: case 0x19: case 0x1C:
				sig.PutU8((u8)kind);
				return;
			case 0x0F:
			{
				int inner = m_binary.FindTypeByPtr(datapoint);
				sig.PutU8(0x0F);
				EncodeType(inner, sig, depth + 1);
				return;
			}
			case 0x11:
			case 0x12:
			case 0x55:
			{
				bool is_value = (kind == 0x11);
				if (kind == 0x55)
				{
					int td = m_binary.TypedefFromType(type_idx);
					if (td >= 0 && (size_t)td < m_metadata.Typedefs().size())
						is_value = m_metadata.Typedefs()[(size_t)td].is_valuetype;
					else
						is_value = true;
				}
				sig.PutU8(is_value ? 0x11 : 0x12);
				sig.PutCoded(TypeDefOrRef(type_idx));
				return;
			}
			case 0x15:
			{
				u64 raw = m_binary.RawFromVa(datapoint);
				int generic_type = -1;
				u64 class_inst_ptr = 0;
				if (raw != UINT64_MAX && m_binary.BinaryHas(raw, 24))
				{
					generic_type = m_binary.FindTypeByPtr(m_binary.BinaryReadU64(raw));
					class_inst_ptr = m_binary.BinaryReadU64(raw + 8);
				}
				int generic_typedef = m_binary.TypedefFromType(generic_type);
				if (generic_typedef < 0)
				{
					sig.PutU8(0x1C);
					return;
				}
				u32 gdb_kind = m_binary.BinaryTypeKind(generic_type);
				sig.PutU8(0x15);
				sig.PutU8(gdb_kind == 0x11 ? 0x11 : 0x12);
				sig.PutCoded(TypeDefOrRef(generic_type));
				u64 inst_raw = m_binary.RawFromVa(class_inst_ptr);
				i64 argc = 0;
				u64 argv = 0;
				if (inst_raw != UINT64_MAX && m_binary.BinaryHas(inst_raw, 16))
				{
					argc = m_binary.BinaryReadI64(inst_raw);
					argv = m_binary.BinaryReadU64(inst_raw + 8);
				}
				if (argc <= 0 || argc > 16)
				{
					sig.PutU8(0x1C);
					return;
				}
				sig.PutCompressed((u32)argc);
				for (i64 i = 0; i < argc; i++)
				{
					u64 arg_ptr = m_binary.ReadVaU64(argv + (u64)i * 8);
					EncodeType(m_binary.FindTypeByPtr(arg_ptr), sig, depth + 1);
				}
				return;
			}
			case 0x13:
				sig.PutU8(0x13);
				sig.PutCompressed((u32)datapoint);
				return;
			case 0x1E:
				sig.PutU8(0x1E);
				sig.PutCompressed((u32)datapoint);
				return;
			case 0x1D:
			{
				int inner = m_binary.FindTypeByPtr(datapoint);
				sig.PutU8(0x1D);
				EncodeType(inner, sig, depth + 1);
				return;
			}
			case 0x14:
			{
				u64 raw = m_binary.RawFromVa(datapoint);
				int inner = -1;
				u32 rank = 1;
				if (raw != UINT64_MAX && m_binary.BinaryHas(raw, 16))
				{
					inner = m_binary.FindTypeByPtr(m_binary.BinaryReadU64(raw));
					rank = m_binary.BinaryReadU8(raw + 8);
					if (rank == 0)
						rank = 1;
				}
				sig.PutU8(0x14);
				EncodeType(inner, sig, depth + 1);
				sig.PutCompressed(rank);
				sig.PutCompressed(0);
				sig.PutCompressed(0);
				return;
			}
			default:
				sig.PutU8(0x1C);
				return;
			}
		}

		void EncodeTypeByref(int type_idx, S_Builder& sig)
		{
			EncodeType(type_idx, sig, 0);
			if (m_binary.BinaryTypeByref(type_idx))
				sig.PutU8(0x10);
		}

		int ParentTypedef(int typedef_index) const
		{
			const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
			if (typedef_index < 0 || (size_t)typedef_index >= typedefs.size())
				return -1;
			int declaring = typedefs[(size_t)typedef_index].declaring;
			if (declaring < 0)
				return -1;
			return m_binary.TypedefFromType(declaring);
		}

		void WriteTypedefRow(int typedef_index)
		{
			const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
			const std::vector<S_MethodDef>& methods = m_metadata.Methods();
			const std::vector<S_FieldDef>& fields = m_metadata.Fields();
			const std::vector<S_ParamDef>& params = m_metadata.Params();
			const S_TypeDef& type = typedefs[(size_t)typedef_index];

			int enclosing = ParentTypedef(typedef_index);
			bool nested = (enclosing >= 0 && TypedefImage(enclosing) == m_image);
			std::string name = m_metadata.GetString(type.name_idx);
			std::string ns = nested ? "" : m_metadata.GetString(type.namespace_idx);

			u32 extends = 0;
			if (!(type.flags & 0x20u) && typedef_index != 0)
			{
				if (type.parent >= 0)
				{
					std::string parent_name = m_binary.TypeName(type.parent, typedef_index, -1);
					if (!type.is_valuetype && !type.is_enum && parent_name != "object")
					{
						u64 datapoint = 0;
						u32 kind = 0;
						if (m_binary.BinaryTypeRaw(type.parent, datapoint, kind) && kind == 0x15)
							extends = ((u32)FindTypeSpec(type.parent) << 2) | 2;
						else
							extends = TypeDefOrRef(type.parent);
					}
					else if (type.is_enum)
						extends = ((u32)FindTypeRef("mscorlib", "System", "Enum") << 2) | 1;
					else if (type.is_valuetype)
						extends = ((u32)FindTypeRef("mscorlib", "System", "ValueType") << 2) | 1;
					else
						extends = ((u32)ObjectRef() << 2) | 1;
				}
				else
					extends = ((u32)ObjectRef() << 2) | 1;
			}

			u32 field_rid = (u32)m_field_count + 1;
			u32 method_rid = (u32)m_method_count + 1;
			m_typedef.PutU32(type.flags);
			m_typedef.PutStr(m_strings.Add(name));
			m_typedef.PutStr(m_strings.Add(ns));
			PutCoded(m_typedef, extends, m_w_typedef_or_ref);
			PutRid(m_typedef, field_rid);
			PutRid(m_typedef, method_rid);

			if (nested)
			{
				PutRid(m_nested, (u32)m_typedef_rids[(size_t)typedef_index]);
				PutRid(m_nested, (u32)m_typedef_rids[(size_t)enclosing]);
				m_nested_count++;
			}

			for (int i = 0; i < type.field_count; i++)
			{
				int global = type.field_start + i;
				if (global < 0 || (size_t)global >= fields.size())
					break;
				WriteFieldRow(fields[(size_t)global], global);
			}
			for (int i = 0; i < type.method_count; i++)
			{
				int global = type.method_start + i;
				if (global < 0 || (size_t)global >= methods.size())
					break;
				WriteMethodRow(methods[(size_t)global], global);
			}

			if (type.generic_container >= 0 && (size_t)type.generic_container < m_metadata.GenericContainers().size())
			{
				const S_GenericContainer& gc = m_metadata.GenericContainers()[(size_t)type.generic_container];
				const std::vector<S_GenericParam>& gparams = m_metadata.GenericParams();
				for (int i = 0; i < gc.argc; i++)
				{
					size_t at = (size_t)gc.param_start + i;
					if (at >= gparams.size())
						break;
					m_genericparam.PutU16((u16)gparams[at].num);
					m_genericparam.PutU16(0);
					PutCoded(m_genericparam, ((u32)m_typedef_rids[(size_t)typedef_index] << 1) | 0, m_w_type_or_method);
					m_genericparam.PutStr(m_strings.Add(m_metadata.GetString(gparams[at].name_idx)));
					m_genericparam_count++;
				}
			}
			(void)params;
		}

		u32 FindTypeSpec(int type_idx)
		{
			u64 datapoint = 0;
			u32 kind = 0;
			if (!m_binary.BinaryTypeRaw(type_idx, datapoint, kind))
				return 0;
			auto it = m_typespec_map.find(type_idx);
			if (it != m_typespec_map.end())
				return it->second;
			S_Builder sig;
			EncodeType(type_idx, sig, 0);
			u32 rid = ++m_typespec_count;
			m_typespec.PutBlobRef(m_blob.Add(sig));
			m_typespec_map[type_idx] = rid;
			return rid;
		}

		void WriteFieldRow(const S_FieldDef& field, int global)
		{
			u32 fattrs = m_binary.BinaryTypeAttrs(field.type_idx);
			S_Builder sig;
			sig.PutU8(0x06);
			EncodeType(field.type_idx, sig, 0);
			m_field.PutU16((u16)(fattrs & 0xFFFFu));
			m_field.PutStr(m_strings.Add(m_metadata.GetString(field.name_idx)));
			m_field.PutBlobRef(m_blob.Add(sig));
			u32 field_rid = ++m_field_count;

			if (fattrs & 0x40u)
			{
				S_DefaultValue def;
				if (m_metadata.FieldDefault(global, def) && def.data_idx != -1)
				{
					u8 etype = 0;
					S_Builder value;
					if (ReadConstBlob(def.type_idx, def.data_idx, value, etype))
					{
						m_constant.PutU8(etype);
						m_constant.PutU8(0);
						PutCoded(m_constant, (field_rid << 2) | 0, m_w_has_const);
						m_constant.PutBlobRef(m_blob.Add(value));
						m_constant_count++;
					}
				}
			}
		}

		bool ReadConstBlob(int const_type, int data_idx, S_Builder& value, u8& etype)
		{
			u32 kind = m_binary.BinaryTypeKind(const_type);
			if (kind == 0x11 || kind == 0x12 || kind == 0x55)
			{
				int td = m_binary.TypedefFromType(const_type);
				if (td < 0 || (size_t)td >= m_metadata.Typedefs().size())
					return false;
				kind = m_binary.BinaryTypeKind(m_metadata.Typedefs()[(size_t)td].element);
			}
			if (data_idx < 0)
				return false;
			u64 pos = m_metadata.DefaultDataBase() + (u64)(i64)data_idx;
			u64 end = m_metadata.RawSize();
			bool ok = false;
			switch (kind)
			{
			case 0x02:
				if (!m_metadata.RawHas(pos, 1)) return false;
				etype = 0x02;
				value.PutU8(m_metadata.ReadRawU8(pos) ? 1 : 0);
				return true;
			case 0x03:
				if (!m_metadata.RawHas(pos, 2)) return false;
				etype = 0x03;
				value.PutU16(m_metadata.ReadRawU16(pos));
				return true;
			case 0x04: case 0x05:
				if (!m_metadata.RawHas(pos, 1)) return false;
				etype = (u8)kind;
				value.PutU8(m_metadata.ReadRawU8(pos));
				return true;
			case 0x06: case 0x07:
				if (!m_metadata.RawHas(pos, 2)) return false;
				etype = (u8)kind;
				value.PutU16(m_metadata.ReadRawU16(pos));
				return true;
			case 0x08:
			{
				i32 v = m_metadata.ReadCompressedI32(pos, end, ok);
				if (!ok) return false;
				etype = 0x08;
				value.PutU32((u32)v);
				return true;
			}
			case 0x09:
			{
				u32 v = m_metadata.ReadCompressedU32(pos, end, ok);
				if (!ok) return false;
				etype = 0x09;
				value.PutU32(v);
				return true;
			}
			case 0x0A: case 0x0B:
				if (!m_metadata.RawHas(pos, 8)) return false;
				etype = (u8)kind;
				value.PutU64(m_metadata.ReadRawU64(pos));
				return true;
			case 0x0C:
				if (!m_metadata.RawHas(pos, 4)) return false;
				etype = 0x0C;
				value.PutU32(m_metadata.ReadRawU32(pos));
				return true;
			case 0x0D:
				if (!m_metadata.RawHas(pos, 8)) return false;
				etype = 0x0D;
				value.PutU64(m_metadata.ReadRawU64(pos));
				return true;
			default:
				return false;
			}
		}

		void WriteMethodRow(const S_MethodDef& method, int global)
		{
			const std::vector<S_ParamDef>& params = m_metadata.Params();
			u32 flags = method.flags | 0x80u;
			std::string name = m_metadata.GetString(method.name_idx);
			if (name == ".ctor" || name == ".cctor")
				flags |= 0x1800u;
			S_Builder sig;
			u8 header = (method.flags & 0x10u) ? 0x00 : 0x20;
			int gen_argc = 0;
			if (method.generic_container >= 0 && (size_t)method.generic_container < m_metadata.GenericContainers().size())
				gen_argc = m_metadata.GenericContainers()[(size_t)method.generic_container].argc;
			if (gen_argc > 0)
				header |= 0x10;
			sig.PutU8(header);
			if (gen_argc > 0)
				sig.PutCompressed((u32)gen_argc);
			sig.PutCompressed((u32)(method.param_count < 0 ? 0 : method.param_count));
			EncodeTypeByref(method.return_type, sig);
			for (int j = 0; j < method.param_count; j++)
			{
				int param_global = method.param_start + j;
				if (param_global < 0 || (size_t)param_global >= params.size())
				{
					sig.PutU8(0x1C);
					continue;
				}
				EncodeTypeByref(params[(size_t)param_global].type_idx, sig);
			}
			u32 param_rid = (u32)m_param_count + 1;
			m_method.PutU32(0);
			m_method.PutU16(0);
			m_method.PutU16((u16)(flags & 0xFFFFu));
			m_method.PutStr(m_strings.Add(name));
			m_method.PutBlobRef(m_blob.Add(sig));
			PutRid(m_method, param_rid);
			u32 method_rid = ++m_method_count;

			for (int j = 0; j < method.param_count; j++)
			{
				int param_global = method.param_start + j;
				if (param_global < 0 || (size_t)param_global >= params.size())
					break;
				const S_ParamDef& param = params[(size_t)param_global];
				u32 pattrs = m_binary.BinaryTypeAttrs(param.type_idx);
				m_param.PutU16((u16)(pattrs & 0xFFFFu));
				m_param.PutU16((u16)(j + 1));
				m_param.PutStr(m_strings.Add(m_metadata.GetString(param.name_idx)));
				m_param_count++;
			}

			if (method.generic_container >= 0 && (size_t)method.generic_container < m_metadata.GenericContainers().size())
			{
				const S_GenericContainer& gc = m_metadata.GenericContainers()[(size_t)method.generic_container];
				const std::vector<S_GenericParam>& gparams = m_metadata.GenericParams();
				for (int i = 0; i < gc.argc; i++)
				{
					size_t at = (size_t)gc.param_start + i;
					if (at >= gparams.size())
						break;
					m_genericparam.PutU16((u16)gparams[at].num);
					m_genericparam.PutU16(0);
					PutCoded(m_genericparam, (method_rid << 1) | 1, m_w_type_or_method);
					m_genericparam.PutStr(m_strings.Add(m_metadata.GetString(gparams[at].name_idx)));
					m_genericparam_count++;
				}
			}
			(void)global;
		}

		std::vector<u8> WritePe(const std::string& file_name)
		{
			u8 heap_sizes = 0x05;

			S_Builder heap;
			heap.PutU32(0);
			heap.PutU8(2);
			heap.PutU8(0);
			heap.PutU8(heap_sizes);
			heap.PutU8(1);
			u64 valid = 0;
			struct S_Table { int index; S_Builder* data; u32 rows; };
			std::vector<S_Table> tables;
			if (true) { tables.push_back({ 0, &m_module, 1 }); valid |= (1ULL << 0); }
			if (m_typeref_count) { tables.push_back({ 1, &m_typeref, m_typeref_count }); valid |= (1ULL << 1); }
			if (m_typedef_count) { tables.push_back({ 2, &m_typedef, m_typedef_count }); valid |= (1ULL << 2); }
			if (m_field_count) { tables.push_back({ 4, &m_field, m_field_count }); valid |= (1ULL << 4); }
			if (m_method_count) { tables.push_back({ 6, &m_method, m_method_count }); valid |= (1ULL << 6); }
			if (m_param_count) { tables.push_back({ 8, &m_param, m_param_count }); valid |= (1ULL << 8); }
			if (m_constant_count) { tables.push_back({ 11, &m_constant, m_constant_count }); valid |= (1ULL << 11); }
			{ tables.push_back({ 32, &m_assembly, 1 }); valid |= (1ULL << 32); }
			if (m_assemblyref_count) { tables.push_back({ 35, &m_assemblyref, m_assemblyref_count }); valid |= (1ULL << 35); }
			if (m_nested_count) { tables.push_back({ 41, &m_nested, m_nested_count }); valid |= (1ULL << 41); }
			if (m_typespec_count) { tables.push_back({ 43, &m_typespec, m_typespec_count }); valid |= (1ULL << 43); }
			if (m_genericparam_count) { tables.push_back({ 44, &m_genericparam, m_genericparam_count }); valid |= (1ULL << 44); }
			heap.PutU64(valid);
			heap.PutU64(valid);
			for (size_t i = 0; i < tables.size(); i++)
				heap.PutU32(tables[i].rows);
			for (size_t i = 0; i < tables.size(); i++)
				heap.PutBuilder(*tables[i].data);

			S_Builder md;
			md.PutU32(0x424A5342u);
			md.PutU16(1);
			md.PutU16(1);
			md.PutU32(0);
			std::string version_text = "v4.0.30319";
			md.PutU32(12);
			md.PutString(version_text);
			md.PutU8(0);
			md.PutU8(0);
			md.PutU16(0);
			md.PutU16(5);
			u32 md_base = (u32)md.Size() + 76;
			u32 heap_off = md_base;
			u32 strings_off = heap_off + (u32)heap.Size();
			u32 us_off = strings_off + (u32)m_strings.data.Size();
			u32 guid_off = us_off + (u32)m_us.data.Size();
			u32 blob_off = guid_off + 16;
			md.PutU32(heap_off);
			md.PutU32((u32)heap.Size());
			md.PutString("#~");
			md.PutU8(0);
			md.PutU8(0);
			md.PutU32(strings_off);
			md.PutU32((u32)m_strings.data.Size());
			md.PutString("#Strings");
			md.PutU8(0);
			md.PutU8(0);
			md.PutU8(0);
			md.PutU8(0);
			md.PutU32(us_off);
			md.PutU32((u32)m_us.data.Size());
			md.PutString("#US");
			md.PutU8(0);
			md.PutU32(guid_off);
			md.PutU32(16);
			md.PutString("#GUID");
			md.PutU8(0);
			md.PutU8(0);
			md.PutU8(0);
			md.PutU32(blob_off);
			md.PutU32((u32)m_blob.data.Size());
			md.PutString("#Blob");
			md.PutU8(0);
			md.PutU8(0);
			md.PutU8(0);
			md.PutBuilder(heap);
			md.PutBuilder(m_strings.data);
			md.PutBuilder(m_us.data);
			{
				u32 hash = 0x811C9DC5u;
				for (size_t i = 0; i < file_name.size(); i++)
				{
					hash ^= (u8)file_name[i];
					hash *= 0x01000193u;
				}
				for (int i = 0; i < 16; i++)
				{
					hash ^= (u32)(i * 0x9E3779B1u);
					hash *= 0x01000193u;
					md.PutU8((u8)(hash & 0xFF));
				}
			}
			md.PutBuilder(m_blob.data);

			S_Builder text;
			for (int i = 0; i < 72; i++)
				text.PutU8(0);
			text.PutBuilder(md);

			u32 text_size = (u32)text.Size();
			u32 text_raw_size = (text_size + 0x1FF) & ~0x1FFu;
			u32 text_va = 0x2000;
			u32 cli_rva = text_va;

			S_Builder cli;
			cli.PutU32(72);
			cli.PutU16(2);
			cli.PutU16(5);
			cli.PutU32(text_va + 72);
			cli.PutU32((u32)md.Size());
			cli.PutU32(0x08);
			cli.PutU32(0);
			for (int i = 0; i < 12; i++)
				cli.PutU32(0);
			for (size_t i = 0; i < cli.bytes.size() && i < text.bytes.size(); i++)
				text.bytes[i] = cli.bytes[i];

			S_Builder pe;
			pe.PutU8('M');
			pe.PutU8('Z');
			for (int i = 0; i < 58; i++)
				pe.PutU8(0);
			pe.PutU32(0x80);
			while (pe.Size() < 0x80)
				pe.PutU8(0);
			pe.PutU32(0x4550u);
			pe.PutU16(0x14C);
			pe.PutU16(1);
			pe.PutU32(0);
			pe.PutU32(0);
			pe.PutU32(0);
			pe.PutU16(0xE0);
			pe.PutU16(0x210E);
			pe.PutU16(0x10B);
			pe.PutU8(0);
			pe.PutU8(0);
			pe.PutU32(text_raw_size);
			pe.PutU32(0);
			pe.PutU32(0);
			pe.PutU32(0);
			pe.PutU32(text_va);
			pe.PutU32(text_va);
			pe.PutU32(0x10000000u);
			pe.PutU32(0x2000);
			pe.PutU32(0x200);
			pe.PutU16(4);
			pe.PutU16(0);
			pe.PutU16(0);
			pe.PutU16(0);
			pe.PutU16(4);
			pe.PutU16(0);
			pe.PutU32(0);
			u32 image_size = ((text_va + text_size) + 0x1FFF) & ~0x1FFFu;
			pe.PutU32(image_size);
			pe.PutU32(0x200);
			pe.PutU32(0);
			pe.PutU16(3);
			pe.PutU16(0x8540);
			pe.PutU32(0x100000);
			pe.PutU32(0x1000);
			pe.PutU32(0x100000);
			pe.PutU32(0x1000);
			pe.PutU32(0);
			pe.PutU32(16);
			for (int i = 0; i < 14; i++)
			{
				pe.PutU32(0);
				pe.PutU32(0);
			}
			pe.PutU32(cli_rva);
			pe.PutU32(72);
			pe.PutU32(0);
			pe.PutU32(0);
			pe.PutString(".text");
			pe.PutU8(0);
			pe.PutU8(0);
			pe.PutU8(0);
			pe.PutU32(text_size);
			pe.PutU32(text_va);
			pe.PutU32(text_raw_size);
			pe.PutU32(0x200);
			pe.PutU32(0);
			pe.PutU32(0);
			pe.PutU16(0);
			pe.PutU16(0);
			pe.PutU32(0x60000020u);
			while (pe.Size() < 0x200)
				pe.PutU8(0);
			pe.PutBuilder(text);
			while (pe.Size() < (size_t)0x200 + text_raw_size)
				pe.PutU8(0);
			return pe.bytes;
		}

		int m_image = -1;
		int m_w_rid = 2;
		int m_w_has_const = 2;
		int m_w_type_or_method = 2;
		int m_w_typedef_or_ref = 2;
		const C_Il2cppMetadata& m_metadata;
		const C_Il2cppBinary& m_binary;
		const std::vector<int>& m_typedef_image;
		std::vector<int> m_typedef_rids;
		std::unordered_map<std::string, u32> m_typeref_map;
		std::unordered_map<std::string, u32> m_assemblyref_map;
		std::unordered_map<int, u32> m_typespec_map;
		S_StringHeap m_strings;
		S_BlobHeap m_blob;
		S_UserStringHeap m_us;
		S_Builder m_module;
		S_Builder m_typeref;
		S_Builder m_typedef;
		S_Builder m_field;
		S_Builder m_method;
		S_Builder m_param;
		S_Builder m_constant;
		S_Builder m_nested;
		S_Builder m_typespec;
		S_Builder m_genericparam;
		S_Builder m_assembly;
		S_Builder m_assemblyref;
		u32 m_typedef_count = 0;
		u32 m_typeref_count = 0;
		u32 m_field_count = 0;
		u32 m_method_count = 0;
		u32 m_param_count = 0;
		u32 m_constant_count = 0;
		u32 m_nested_count = 0;
		u32 m_typespec_count = 0;
		u32 m_genericparam_count = 0;
		u32 m_assemblyref_count = 0;
	};

	C_DummyDll::C_DummyDll(const C_Il2cppMetadata& metadata, const C_Il2cppBinary& binary)
		: m_metadata(metadata), m_binary(binary)
	{
	}

	bool C_DummyDll::Generate(const std::string& directory)
	{
		CreateDirectoryA(directory.c_str(), nullptr);
		const std::vector<S_ImageDef>& images = m_metadata.Images();
		const std::vector<S_TypeDef>& typedefs = m_metadata.Typedefs();
		std::vector<int> typedef_image(typedefs.size(), -1);
		for (size_t i = 0; i < images.size(); i++)
		{
			int type_end = images[i].start + (int)images[i].count;
			for (int t = images[i].start; t < type_end; t++)
			{
				if (t < 0 || (size_t)t >= typedefs.size())
					break;
				typedef_image[(size_t)t] = (int)i;
			}
		}
		u32 done = 0;
		const std::vector<S_MethodDef>& methods = m_metadata.Methods();
		const std::vector<S_FieldDef>& fields = m_metadata.Fields();
		const std::vector<S_ParamDef>& params = m_metadata.Params();
		for (size_t i = 0; i < images.size(); i++)
		{
			u32 type_total = images[i].count;
			u32 field_total = 0;
			u32 method_total = 0;
			u32 param_total = 0;
			int type_end = images[i].start + (int)images[i].count;
			for (int t = images[i].start; t < type_end; t++)
			{
				if (t < 0 || (size_t)t >= typedefs.size())
					break;
				const S_TypeDef& type = typedefs[(size_t)t];
				field_total += (u32)(type.field_count < 0 ? 0 : type.field_count);
				method_total += (u32)(type.method_count < 0 ? 0 : type.method_count);
				for (int k = 0; k < type.method_count; k++)
				{
					int global = type.method_start + k;
					if (global < 0 || (size_t)global >= methods.size())
						break;
					param_total += (u32)(methods[(size_t)global].param_count < 0 ? 0 : methods[(size_t)global].param_count);
				}
			}
			(void)fields;
			(void)params;
			int w_rid = (type_total >= 65536 || field_total >= 65536 || method_total >= 65536 || param_total >= 65536) ? 4 : 2;
			int w_has_const = (field_total >= 16384 || param_total >= 16384) ? 4 : 2;
			int w_tomd = (type_total >= 32768 || method_total >= 32768) ? 4 : 2;
			int w_tdor = (type_total >= 16384) ? 4 : 2;
			C_DummyAssembly assembly(m_metadata, m_binary, typedef_image);
			assembly.SetWidths(w_rid, w_has_const, w_tomd, w_tdor);
			std::vector<u8> bytes = assembly.Build((int)i);
			std::string file_name = images[i].name;
			if (file_name.size() < 4 || file_name.substr(file_name.size() - 4) != ".dll")
				file_name += ".dll";
			std::string path = directory + "\\" + file_name;
			FILE* file = nullptr;
			if (fopen_s(&file, path.c_str(), "wb") != 0 || !file)
			{
				C_Log::Fail("Cannot write " + path);
				return false;
			}
			fwrite(bytes.data(), 1, bytes.size(), file);
			fclose(file);
			done++;
		}
		C_Log::Ok("Dummy DLLs: " + std::to_string(done) + "/" + std::to_string(images.size()));
		return true;
	}
}
