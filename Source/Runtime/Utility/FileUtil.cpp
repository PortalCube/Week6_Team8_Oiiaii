#include "FileUtil.h"

#include "Runtime/Utility/EngineUtil.h"

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

FString FileUtil::ReadTextFile(FStringView Path)
{
	// std::string_view::data()는 null 종료 문자를 보장하지 않음.
	// 그래서 이걸 std::string::c_str()으로 바꿔야 하나 싶은데..
	// 이거 하나 때문에 FString으로 복사하는 비용이 과연 괜찮은지 모르겠음

	// 일단 지금은 이거 쓸 땐 FStringView를 substr 하지 않아야함 (부디 그런 사례가 없을거라고 믿음)
	// substr 해도 data()로 반환된 값은 null 종료 문자가 없으므로, 그냥 원래의 문자열이 나오거나
	// 최악의 경우엔 지정된 영역을 벗어나는 버퍼 오버런이 발생함

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

FArchive FileUtil::ReadArchive(FStringView Path)
{
	try
	{
		return FArchive{ ReadJSONFile(Path) };
	}
	catch (...)
	{
		throw EngineUtil::CreateError("[FileUtil::ReadArchive] FArchive를 불러오는데 실패했습니다.");
	}
}

void FileUtil::WriteArchive(FStringView Path, const FArchive& Archive)
{
	try
	{
		WriteJSONFile(Path, Archive.GetJSON());
	}
	catch (...)
	{
		throw EngineUtil::CreateError("[FileUtil::WriteArchive] FArchive를 쓰는데 실패했습니다.");
	}
}
