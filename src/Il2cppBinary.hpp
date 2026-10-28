#pragma once
#include <unordered_map>
#include "Il2cppMetadata.hpp"

namespace AvirA
{
	struct S_PeSection
	{
		std::string name;
		u64 virt_addr = 0;
		u64 virt_size = 0;
		u64 raw_ptr = 0;
		u64 raw_size = 0;
		u32 chars = 0;
	};

	struct S_CodeGenModule
	{
		std::string name;
		u64 method_pointer_count = 0;
		u64 method_pointers = 0;
	};

	struct S_GenericInstance
	{
		u64 pointer = 0;
		u64 rva = UINT64_MAX;
		u64 file_offset = UINT64_MAX;
		std::string display_name;
		std::string method_name;
	};

	struct S_Il2CppType
	{
		u64 datapoint = 0;
		u32 attrs = 0;
		u32 kind = 0;
		u32 num_mods = 0;
		u32 byref = 0;
	};

	class C_Il2cppBinary
	{
	public:
		bool Load(const std::string& path);
		bool Resolve(const C_Il2cppMetadata& metadata);

		bool HasRegistration() const;
		u64 ImageBase() const;
		u64 FileSize() const;

		u64 RawFromVa(u64 va) const;
		u64 VaFromRaw(u64 raw) const;
		bool BinaryHas(u64 raw, u64 size) const;
		u64 BinaryReadU64(u64 raw) const;
		i64 BinaryReadI64(u64 raw) const;
		u8 BinaryReadU8(u64 raw) const;
		u64 ReadVaU64(u64 va) const;
		u32 ReadVaU32(u64 va) const;
		u8 ReadVaU8(u64 va) const;

		u64 MethodRva(const std::string& image_name, u32 token) const;
		u64 FileOffsetFromRva(u64 rva) const;
		const std::vector<S_PeSection>& Sections() const;

		i32 FieldOffset(int type_index, int field_index_in_type, bool is_valuetype, bool is_static) const;

		std::string TypeName(int type_index, int context_typedef = -1, int context_method = -1, int depth = 0) const;

		std::vector<S_GenericInstance> GenericInstancesFull(int method_def_index) const;
		int MethodSpecMethodDef(int spec_index) const;
		u64 SpecPointerRva(int spec_index) const;
		std::string GenericInstArgs(u64 inst_va, int context_method) const;

		u32 BinaryTypeKind(int type_index) const;
		u32 BinaryTypeAttrs(int type_index) const;
		u32 BinaryTypeByref(int type_index) const;
		bool BinaryTypeRaw(int type_index, u64& datapoint, u32& kind) const;
		int TypeIndexFromPtr(u64 type_ptr) const;
		int FindTypeByPtr(u64 type_ptr) const;
		int TypedefFromType(int type_index) const;
		std::string NestedDisplayName(int typedef_index, bool for_declaration) const;
		std::string RawNestedName(int typedef_index) const;
		std::string TypedefDeclName(int typedef_index) const;

		const C_Il2cppMetadata* Metadata() const;

	private:
		bool ParsePe();
		bool FindCodeRegistration(const C_Il2cppMetadata& metadata, u64& out_code_reg);
		bool ReadCodeRegistration(u64 code_reg_va, const C_Il2cppMetadata& metadata);
		bool FindMetadataRegistration(const C_Il2cppMetadata& metadata, u64& out_meta_reg);
		bool ReadMetadataRegistration(u64 meta_reg_va, const C_Il2cppMetadata& metadata);
		std::vector<u64> FindRefs(u64 target_va, bool in_data) const;
		bool IsDataVa(u64 va) const;
		bool IsExecVa(u64 va) const;
		std::string GenericParamName(int container_idx, int num, bool is_method) const;
		C_Buffer m_file;
		u64 m_image_base = 0;
		std::vector<S_PeSection> m_sections;

		const C_Il2cppMetadata* m_metadata = nullptr;
		bool m_has_registration = false;
		int m_binary_version = 310;

		std::vector<S_CodeGenModule> m_modules;
		std::vector<u64> m_invoker_pointers;
		std::vector<S_Il2CppType> m_types;
		std::vector<u64> m_type_ptrs;
		std::unordered_map<u64, int> m_type_ptr_map;
		std::vector<u64> m_field_offset_ptrs;

		struct S_MethodSpecRaw { i32 method_def = -1; i32 class_inst = -1; i32 method_inst = -1; };
		struct S_GenericMethodRaw { i32 spec_index = -1; i32 method_index = -1; i32 invoker_index = -1; };
		std::vector<S_MethodSpecRaw> m_method_specs;
		std::vector<S_GenericMethodRaw> m_generic_methods;
		std::unordered_map<int, std::vector<size_t>> m_method_spec_map;
		std::vector<u64> m_generic_method_pointers;
		std::vector<u64> m_generic_insts;
		std::unordered_map<int, u64> m_spec_ptr_map;
	};
}
