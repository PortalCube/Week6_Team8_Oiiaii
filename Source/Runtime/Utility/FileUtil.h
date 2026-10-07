#pragma once

#include "Runtime/Serialization/FJson.h"
#include "Runtime/Core/FString.h"
#include "ThirdParty/Json/json.hpp"


namespace FileUtil
{
	// 엔진 실행 파일이 위치한 디렉토리의 절대 경로 (UTF-8).
	FString GetEngineDirectory();
	// 엔진 실행 파일과 같은 위치의 Content 디렉토리 경로 (UTF-8).
	FString GetContentDirectory();
	// Content 디렉토리에 UTF-8 상대 경로를 결합한 경로를 반환합니다.
	FString GetContentPath(FStringView Path);

	FString ReadTextFile(FStringView Path);
	void WriteTextFile(FStringView Path, const FString& Text);

	nlohmann::json ReadJSONFile(FStringView Path);
	void WriteJSONFile(FStringView Path, const nlohmann::json& JSON);

	FJson ReadJson(FStringView Path);
	void WriteJson(FStringView Path, const FJson& Archive);
}
