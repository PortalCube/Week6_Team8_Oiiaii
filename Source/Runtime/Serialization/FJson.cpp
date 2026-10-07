#include "FJson.h"

#include "Runtime/Utility/WindowsUtil.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

FJson::FJson()
	: Object()
{
}

FJson::FJson(const nlohmann::json& InObject)
	: Object(InObject)
{
}

int32 FJson::GetInt32(const FString& Key) const
{
	return Object.at(Key).get<int32>();
}

void FJson::SetInt32(const FString& Key, int32 Value)
{
	Object[Key] = Value;
}

float FJson::GetFloat(const FString& Key) const
{
	return Object.at(Key).get<float>();
}

void FJson::SetFloat(const FString& Key, float Value)
{
	Object[Key] = Value;
}

uint32 FJson::GetUInt32(const FString& Key) const
{
	return Object.at(Key).get<uint32>();
}

void FJson::SetUInt32(const FString& Key, uint32 Value)
{
	Object[Key] = Value;
}

double FJson::GetDouble(const FString& Key) const
{
	return Object.at(Key).get<double>();
}

void FJson::SetDouble(const FString& Key, double Value)
{
	Object[Key] = Value;
}

bool FJson::GetBool(const FString& Key) const
{
	return Object.at(Key).get<bool>();
}

void FJson::SetBool(const FString& Key, bool Value)
{
	Object[Key] = Value;
}

FString FJson::GetString(const FString& Key) const
{
	return Object.at(Key).get<FString>();
}

void FJson::SetString(const FString& Key, const FString& Value)
{
	Object[Key] = Value;
}

FWString FJson::GetWString(const FString& Key) const
{
	FString Result = Object.at(Key).get<FString>();
	return WindowsUtil::ToWString(Result);
}

void FJson::SetWString(const FString& Key, const FWString& Value)
{
	FString Result = WindowsUtil::ToString(Value);
	Object[Key] = Result;
}

bool FJson::IsNull(const FString& Key) const
{
	// 주어진 키 자체가 존재하지 않음
	if (!Object.contains(Key))
	{
		return true;
	}

	// 주어진 키의 value가 null 값임
	if (Object.at(Key).is_null())
	{
		return true;
	}

	// 값이 있음
	return false;
}

void FJson::SetNull(const FString& Key)
{
	// 참고: IsNull과는 다르게, SetNull은 반드시 명시적인 null을 지정함
	Object[Key] = nullptr;
}

FVector FJson::GetVector(const FString& Key) const
{
	TArray<float> Array = GetArray<float>(Key);

	return FVector{
		Array[0],
		Array[1],
		Array[2],
	};
}

void FJson::SetVector(const FString& Key, const FVector& Value)
{
	TArray<float> Array{
		Value.X,
		Value.Y,
		Value.Z,
	};

	SetArray(Key, Array);
}

FVector2 FJson::GetVector2(const FString& Key) const
{
	TArray<float> Array = GetArray<float>(Key);

	return FVector2{
		Array[0],
		Array[1],
	};
}

void FJson::SetVector2(const FString& Key, const FVector2& Value)
{
	TArray<float> Array{
		Value.X,
		Value.Y,
	};

	SetArray(Key, Array);
}

FVector4 FJson::GetVector4(const FString& Key) const
{
	TArray<float> Array = GetArray<float>(Key);

	return FVector4{
		Array[0],
		Array[1],
		Array[2],
		Array[3],
	};
}

void FJson::SetVector4(const FString& Key, const FVector4& Value)
{
	TArray<float> Array{
		Value.X,
		Value.Y,
		Value.Z,
		Value.W,
	};

	SetArray(Key, Array);
}

TArray<FJson> FJson::GetJsonArray(const FString& Key) const
{
	TArray<FJson> Array;

	for (const auto& Item : Object.at(Key))
	{
		FJson ItemArchive{ Item };
		Array.push_back(ItemArchive);
	}

	return Array;
}

void FJson::SetJsonArray(const FString& Key, const TArray<FJson>& Value)
{
	Object[Key] = nlohmann::json::array();

	for (const auto& Item : Value)
	{
		Object.at(Key).push_back(Item.GetJSON());
	}
}

FJson FJson::GetJson(const FString& Key) const
{
	return FJson{ Object.at(Key) };
}

void FJson::SetJson(const FString& Key, const FJson& Archive)
{
	Object[Key] = Archive.GetJSON();
}
