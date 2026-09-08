#include "DumpWriter.hpp"
#include "DummyDll.hpp"
#include <Windows.h>
#include <cstdlib>

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

struct S_GameTarget
{
	std::string name;
	std::string game_assembly;
	std::string metadata;
};

static std::vector<std::string> SteamLibraries()
{
	std::vector<std::string> out;
	char program_files[MAX_PATH] = {};
	if (GetEnvironmentVariableA("ProgramFiles(x86)", program_files, MAX_PATH) == 0)
		return out;
	std::string steam = std::string(program_files) + "\\Steam";
	out.push_back(steam);
	FILE* file = nullptr;
	std::string vdf = steam + "\\steamapps\\libraryfolders.vdf";
	if (fopen_s(&file, vdf.c_str(), "rb") != 0 || !file)
		return out;
	std::fseek(file, 0, SEEK_END);
	long size = std::ftell(file);
	std::fseek(file, 0, SEEK_SET);
	std::string text;
	if (size > 0 && size < 1000000)
	{
		text.resize((size_t)size);
		text.assign(text.size(), 0);
		size_t got = std::fread(&text[0], 1, text.size(), file);
		text.resize(got);
	}
	std::fclose(file);
	size_t at = 0;
	while ((at = text.find("\"path\"", at)) != std::string::npos)
	{
		at += 6;
		size_t first = text.find('"', at);
		if (first == std::string::npos)
			break;
		size_t second = text.find('"', first + 1);
		if (second == std::string::npos)
			break;
		std::string lib = text.substr(first + 1, second - first - 1);
		std::string unescaped;
		for (size_t i = 0; i < lib.size(); i++)
		{
			if (lib[i] == '\\' && i + 1 < lib.size() && lib[i + 1] == '\\')
			{
				unescaped.push_back('\\');
				i++;
			}
			else
				unescaped.push_back(lib[i]);
		}
		bool known = false;
		for (size_t i = 0; i < out.size(); i++)
		{
			if (_stricmp(out[i].c_str(), unescaped.c_str()) == 0)
			{
				known = true;
				break;
			}
		}
		if (!unescaped.empty() && !known)
			out.push_back(unescaped);
		at = second + 1;
	}
	return out;
}

static std::vector<S_GameTarget> FindGames()
{
	std::vector<S_GameTarget> out;
	std::vector<std::string> libraries = SteamLibraries();
	for (size_t l = 0; l < libraries.size(); l++)
	{
		std::string common = libraries[l] + "\\steamapps\\common";
		WIN32_FIND_DATAA found = {};
		HANDLE handle = FindFirstFileA((common + "\\*").c_str(), &found);
		if (handle == INVALID_HANDLE_VALUE)
			continue;
		do
		{
			if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;
			std::string dir = found.cFileName;
			if (dir == "." || dir == "..")
				continue;
			std::string game_assembly = common + "\\" + dir + "\\GameAssembly.dll";
			if (!FileExists(game_assembly))
				continue;
			std::string game_dir = common + "\\" + dir;
			WIN32_FIND_DATAA found_data = {};
			HANDLE handle_data = FindFirstFileA((game_dir + "\\*_Data").c_str(), &found_data);
			if (handle_data == INVALID_HANDLE_VALUE)
				continue;
			do
			{
				if (!(found_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					continue;
				std::string metadata = game_dir + "\\" + found_data.cFileName + "\\il2cpp_data\\Metadata\\global-metadata.dat";
				if (FileExists(metadata))
				{
					S_GameTarget target;
					target.name = dir;
					target.game_assembly = game_assembly;
					target.metadata = metadata;
					out.push_back(target);
					break;
				}
			} while (FindNextFileA(handle_data, &found_data));
			FindClose(handle_data);
		} while (FindNextFileA(handle, &found));
		FindClose(handle);
	}
	return out;
}

static std::string CleanName(const std::string& name)
{
	std::string out;
	for (size_t i = 0; i < name.size(); i++)
	{
		char c = name[i];
		bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
			|| c == ' ' || c == '.' || c == '-' || c == '_' || c == '(' || c == ')';
		out.push_back(ok ? c : '_');
	}
	if (out.empty())
		out = "Game";
	return out;
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
	std::string game_name;
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
	if (game_assembly.empty() || metadata_path.empty())
	{
		std::vector<S_GameTarget> games = FindGames();
		if (games.empty())
		{
			AvirA::C_Log::Fail("No IL2CPP games found. Pass GameAssembly.dll and global-metadata.dat as arguments.");
			return 1;
		}
		size_t pick = 0;
		if (games.size() == 1)
		{
			AvirA::C_Log::Info("Found game: " + games[0].name);
		}
		else
		{
			AvirA::C_Log::Line("Found games:");
			for (size_t i = 0; i < games.size(); i++)
				AvirA::C_Log::Line("  [" + std::to_string(i + 1) + "] " + games[i].name);
			while (true)
			{
				std::printf("Select game [1-%llu] (default 1): ", (unsigned long long)games.size());
				std::fflush(stdout);
				char answer[32] = {};
				if (!std::fgets(answer, sizeof(answer), stdin))
					break;
				int number = std::atoi(answer);
				if (number >= 1 && (size_t)number <= games.size())
				{
					pick = (size_t)number - 1;
					break;
				}
				AvirA::C_Log::Fail("Bad number, try again.");
			}
		}
		game_assembly = games[pick].game_assembly;
		metadata_path = games[pick].metadata;
		game_name = games[pick].name;
	}
	if (!out_custom || !IsAbsolute(out_dir))
	{
		std::string base = ExeDir();
		if (!out_custom)
		{
			if (game_name.empty())
				out_dir = base + "\\Dump";
			else
				out_dir = base + "\\Dump_" + CleanName(game_name);
		}
		else
			out_dir = base + "\\" + out_dir;
	}
	AvirA::C_Log::Info("Output: " + out_dir);
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
	AvirA::C_Log::Info("Writing Il2cppDump.cs...");
	std::string dump_path = out_dir + "\\Il2cppDump.cs";
	AvirA::C_DumpWriter writer(metadata, binary);
	if (!writer.Write(dump_path))
	{
		AvirA::C_Log::Fail("Failed to write dump");
		return 1;
	}
	AvirA::C_Log::Ok("Wrote " + dump_path);

	AvirA::C_Log::Info("Writing DummyDll/*.dll...");
	AvirA::C_DummyDll dummy(metadata, binary);
	if (!dummy.Generate(out_dir + "\\DummyDll"))
	{
		AvirA::C_Log::Fail("Failed to generate DummyDll");
		return 1;
	}
	AvirA::C_Log::Ok("Done");
	return 0;
}
