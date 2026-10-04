#pragma once

#include "Runtime/Engine/FArchive.h"
#include "Runtime/Core/FString.h"
#include "ThirdParty/Json/json.hpp"


namespace FileUtil
{
	FString ReadTextFile(FStringView Path);
	void WriteTextFile(FStringView Path, const FString& Text);

	nlohmann::json ReadJSONFile(FStringView Path);
	void WriteJSONFile(FStringView Path, const nlohmann::json& JSON);

	FArchive ReadArchive(FStringView Path);
	void WriteArchive(FStringView Path, const FArchive& Archive);
}
