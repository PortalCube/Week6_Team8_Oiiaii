#include "FileUtil.h"

#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Utility/WindowsUtil.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

FString FileUtil::GetEngineDirectory()
{
	FWString ExecutablePath(MAX_PATH, L'\0');
	while (true)
	{
		const DWORD PathLength = GetModuleFileNameW(
		    nullptr, ExecutablePath.data(),
		    static_cast<DWORD>(ExecutablePath.size()));

		if (PathLength == 0)
		{
			throw EngineUtil::CreateError("[FileUtil::GetEngineDirectory] 실행 파일 경로를 찾지 못했습니다.");
		}

		if (PathLength < ExecutablePath.size())
		{
			ExecutablePath.resize(PathLength);
			return WindowsUtil::ToString(fs::path(ExecutablePath).parent_path().wstring());
		}

		ExecutablePath.resize(ExecutablePath.size() * 2);
	}
}

FString FileUtil::GetContentDirectory()
{
	return WindowsUtil::ToString((fs::u8path(GetEngineDirectory()) / L"Content").wstring());
}

FString FileUtil::GetContentPath(FStringView Path)
{
	return WindowsUtil::ToString((fs::u8path(GetContentDirectory()) / fs::u8path(Path.begin(), Path.end())).wstring());
}

FString FileUtil::ReadTextFile(FStringView Path)
{
	std::ifstream File(Path.data());
	if (!File)
	{
		throw EngineUtil::CreateError("[FileUtil::ReadTextFile] 파일을 읽는데 실패했습니다.");
	}

	std::stringstream Buffer;
	Buffer << File.rdbuf();

	return Buffer.str();
}

void FileUtil::WriteTextFile(FStringView Path, const FString& Text)
{
	fs::path FilePath(Path);
	fs::path Directory = FilePath.parent_path();

	// 디렉토리가 없으면 만듦
	if (!Directory.empty() && !std::filesystem::exists(Directory))
	{
		std::filesystem::create_directories(Directory);
	}

	std::ofstream File(FilePath);
	if (!File)
	{
		throw EngineUtil::CreateError("[FileUtil::WriteTextFile] 파일을 쓰는데 실패했습니다.");
	}

	File << Text;
}

nlohmann::json FileUtil::ReadJSONFile(FStringView Path)
{
	try
	{
		return nlohmann::json::parse(ReadTextFile(Path));
	}
	catch (...)
	{
		throw EngineUtil::CreateError("[FileUtil::ReadJSONFile] JSON 파일을 읽는데 실패했습니다.");
	}
}

void FileUtil::WriteJSONFile(FStringView Path, const nlohmann::json& JSON)
{
	try
	{
		WriteTextFile(Path, JSON.dump(4));
	}
	catch (...)
	{
		throw EngineUtil::CreateError("[FileUtil::WriteJSONFile] JSON 파일을 쓰는데 실패했습니다.");
	}
}

FJson FileUtil::ReadJson(FStringView Path)
{
	return FJson{ ReadJSONFile(Path) };
}

void FileUtil::WriteJson(FStringView Path, const FJson& Json)
{
	WriteJSONFile(Path, Json.GetJSON());
}
