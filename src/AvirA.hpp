#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>

#define AVIRA_DUMPER_NAME "AvirA Dumper Il2cpp"
#define AVIRA_DUMPER_VERSION "1.0.0"
#define AVIRA_METADATA_MAGIC 0xFAB11BAFu
#define AVIRA_METADATA_PATCH_OFFSET 4u
#define AVIRA_METADATA_PATCH_FROM 0x17u
#define AVIRA_METADATA_PATCH_TO 0x1Fu

namespace AvirA
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;
	using i8 = std::int8_t;
	using i16 = std::int16_t;
	using i32 = std::int32_t;
	using i64 = std::int64_t;

	inline std::string FormatHexUpper(u64 value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%llX", (unsigned long long)value);
		return buffer;
	}

	inline std::string FormatU32(u32 value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%u", value);
		return buffer;
	}

	inline std::string FormatI32(i32 value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%d", value);
		return buffer;
	}

	inline std::string FormatI64(i64 value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%lld", (long long)value);
		return buffer;
	}

	inline std::string FormatU64(u64 value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)value);
		return buffer;
	}

	class C_Log
	{
	public:
		static void Ok(const std::string& text)
		{
			std::printf("[+] %s\n", text.c_str());
		}

		static void Info(const std::string& text)
		{
			std::printf("[*] %s\n", text.c_str());
		}

		static void Fail(const std::string& text)
		{
			std::printf("[x] %s\n", text.c_str());
		}

		static void Line(const std::string& text)
		{
			std::printf("%s\n", text.c_str());
		}
	};

	class C_Buffer
	{
	public:
		std::vector<u8> bytes;

		bool LoadFromFile(const std::string& path)
		{
			FILE* file = nullptr;
			if (fopen_s(&file, path.c_str(), "rb") != 0 || !file)
				return false;
			std::fseek(file, 0, SEEK_END);
			long size = std::ftell(file);
			std::fseek(file, 0, SEEK_SET);
			if (size <= 0)
			{
				std::fclose(file);
				return false;
			}
			bytes.resize((size_t)size);
			size_t got = std::fread(bytes.data(), 1, bytes.size(), file);
			std::fclose(file);
			return got == bytes.size();
		}

		bool SaveToFile(const std::string& path) const
		{
			FILE* file = nullptr;
			if (fopen_s(&file, path.c_str(), "wb") != 0 || !file)
				return false;
			size_t put = std::fwrite(bytes.data(), 1, bytes.size(), file);
			std::fclose(file);
			return put == bytes.size();
		}

		size_t Size() const
		{
			return bytes.size();
		}

		const u8* Data() const
		{
			return bytes.data();
		}

		u8* Data()
		{
			return bytes.data();
		}

		bool Has(size_t offset, size_t count) const
		{
			return offset + count <= bytes.size() && offset + count >= offset;
		}

		u16 ReadU16(size_t offset) const
		{
			u16 value = 0;
			if (!Has(offset, 2))
				return value;
			value |= (u16)bytes[offset];
			value |= (u16)((u16)bytes[offset + 1] << 8);
			return value;
		}

		u32 ReadU32(size_t offset) const
		{
			u32 value = 0;
			if (!Has(offset, 4))
				return value;
			value |= (u32)bytes[offset];
			value |= (u32)((u32)bytes[offset + 1] << 8);
			value |= (u32)((u32)bytes[offset + 2] << 16);
			value |= (u32)((u32)bytes[offset + 3] << 24);
			return value;
		}

		i32 ReadI32(size_t offset) const
		{
			return (i32)ReadU32(offset);
		}

		u64 ReadU64(size_t offset) const
		{
			u64 value = 0;
			if (!Has(offset, 8))
				return value;
			value |= (u64)bytes[offset];
			value |= (u64)((u64)bytes[offset + 1] << 8);
			value |= (u64)((u64)bytes[offset + 2] << 16);
			value |= (u64)((u64)bytes[offset + 3] << 24);
			value |= (u64)((u64)bytes[offset + 4] << 32);
			value |= (u64)((u64)bytes[offset + 5] << 40);
			value |= (u64)((u64)bytes[offset + 6] << 48);
			value |= (u64)((u64)bytes[offset + 7] << 56);
			return value;
		}

		i64 ReadI64(size_t offset) const
		{
			return (i64)ReadU64(offset);
		}
	};
}
