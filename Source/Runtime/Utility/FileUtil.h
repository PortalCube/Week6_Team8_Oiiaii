#pragma once

#include "Runtime/Serialization/FJson.h"
#include "Runtime/Core/FString.h"
#include "ThirdParty/Json/json.hpp"


namespace FileUtil
{
	FString ReadTextFile(FStringView Path);
	void WriteTextFile(FStringView Path, const FString& Text);

	nlohmann::json ReadJSONFile(FStringView Path);
	void WriteJSONFile(FStringView Path, const nlohmann::json& JSON);

	FJson ReadJson(FStringView Path);
	void WriteJson(FStringView Path, const FJson& Archive);
}
