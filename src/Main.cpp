#include "DumpWriter.hpp"
#include "DummyDll.hpp"
#include <Windows.h>

static std::string DefaultGameAssembly()
{
	char program_files[MAX_PATH] = {};
	if (GetEnvironmentVariableA("ProgramFiles(x86)", program_files, MAX_PATH) == 0)
		return "";
	std::string path = program_files;
	path += "\\Steam\\steamapps\\common\\SCP Secret Laboratory\\GameAssembly.dll";
	return path;
}

static std::string DefaultMetadata()
{
	char program_files[MAX_PATH] = {};
	if (GetEnvironmentVariableA("ProgramFiles(x86)", program_files, MAX_PATH) == 0)
		return "";
	std::string path = program_files;
	path += "\\Steam\\steamapps\\common\\SCP Secret Laboratory\\SCPSL_Data\\il2cpp_data\\Metadata\\global-metadata.dat";
	return path;
}

static std::string ExeDir()
{
	char path[MAX_PATH] = {};
	DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
	std::string dir(path, path + length);
	size_t slash = dir.find_last_of("\\/");
	if (slash != std::string::npos)
		dir = dir.substr(0, slash);
	if (dir.empty())
		dir = ".";
	return dir;
}

static bool IsAbsolute(const std::string& path)
{
	if (path.size() >= 2 && path[1] == ':')
		return true;
	if (path.size() >= 2 && (path[0] == '\\' || path[0] == '/') && (path[1] == '\\' || path[1] == '/'))
		return true;
	if (!path.empty() && (path[0] == '\\' || path[0] == '/'))
		return true;
	return false;
}
static bool FileExists(const std::string& path)
{
	DWORD attrs = GetFileAttributesA(path.c_str());
	return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

static void CreateDirs(const std::string& path)
{
	std::string part;
	for (size_t i = 0; i <= path.size(); i++)
	{
		if (i == path.size() || path[i] == '\\' || path[i] == '/')
		{
			if (!part.empty() && part.back() != ':')
				CreateDirectoryA(part.c_str(), nullptr);
		}
		if (i < path.size())
			part.push_back(path[i]);
	}
}

int main(int argc, char** argv)
{
	std::printf("%s v%s\n", AVIRA_DUMPER_NAME, AVIRA_DUMPER_VERSION);

	std::string game_assembly;
	std::string metadata_path;
	std::string out_dir;
	bool out_custom = false;
	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		if ((arg == "-o" || arg == "--out") && i + 1 < argc)
		{
			out_dir = argv[++i];
			out_custom = true;
		}
		else if (game_assembly.empty())
			game_assembly = arg;
		else if (metadata_path.empty())
			metadata_path = arg;
	}
	if (!out_custom || !IsAbsolute(out_dir))
	{
		std::string base = ExeDir();
		if (!out_custom)
			out_dir = base + "\\Dump";
		else
			out_dir = base + "\\" + out_dir;
	}
	AvirA::C_Log::Info("Output: " + out_dir);
	if (game_assembly.empty())
		game_assembly = DefaultGameAssembly();
	if (metadata_path.empty())
		metadata_path = DefaultMetadata();
	if (!FileExists(game_assembly))
	{
		AvirA::C_Log::Fail("GameAssembly not found: " + game_assembly);
		return 1;
	}
	if (!FileExists(metadata_path))
	{
		AvirA::C_Log::Fail("Metadata not found: " + metadata_path);
		return 1;
	}
	AvirA::C_Log::Info("GameAssembly: " + game_assembly);
	AvirA::C_Log::Info("Metadata: " + metadata_path);

	AvirA::C_Il2cppMetadata metadata;
	if (!metadata.Load(metadata_path))
	{
		AvirA::C_Log::Fail("Failed to load metadata");
		return 1;
	}
	if (!metadata.ParseHeader())
	{
		AvirA::C_Log::Fail("Bad metadata magic");
		return 1;
	}
	bool is_scpsl = metadata_path.find("SCP Secret Laboratory") != std::string::npos;
	if (is_scpsl && metadata.PatchVersionByte())
	{
		metadata.ParseHeader();
		AvirA::C_Log::Info("Patched metadata byte 0x17 -> 0x1F");
	}
	AvirA::C_Log::Info("Metadata version: " + std::to_string(metadata.Version()));
	if (!metadata.ParseTables())
	{
		AvirA::C_Log::Fail("Failed to parse metadata tables");
		return 1;
	}
	AvirA::C_Log::Ok("Images: " + std::to_string(metadata.Images().size())
		+ " Types: " + std::to_string(metadata.Typedefs().size())
		+ " Methods: " + std::to_string(metadata.Methods().size()));

	AvirA::C_Il2cppBinary binary;
	if (!binary.Load(game_assembly))
	{
		AvirA::C_Log::Fail("Failed to load GameAssembly");
		return 1;
	}
	if (!binary.Resolve(metadata))
	{
		AvirA::C_Log::Fail("Failed to resolve registrations");
		return 1;
	}
	AvirA::C_Log::Ok("Registrations resolved");

	CreateDirs(out_dir);
	CreateDirs(out_dir + "\\DummyDll");
	AvirA::C_Log::Info("Writing dump...");
	std::string dump_path = out_dir + "\\Il2cppDump.cs";
	AvirA::C_DumpWriter writer(metadata, binary);
	if (!writer.Write(dump_path))
	{
		AvirA::C_Log::Fail("Failed to write dump");
		return 1;
	}
	AvirA::C_Log::Ok("Wrote " + dump_path);

	AvirA::C_Log::Info("Generating DLLs...");
	AvirA::C_DummyDll dummy(metadata, binary);
	if (!dummy.Generate(out_dir + "\\DummyDll"))
	{
		AvirA::C_Log::Fail("Failed to generate DummyDll");
		return 1;
	}
	AvirA::C_Log::Ok("Done");
	return 0;
}
