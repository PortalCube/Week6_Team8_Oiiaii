#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Math/FVector4.h"
#include "ThirdParty/Json/json.hpp"

#include "Runtime/Utility/EngineUtil.h"

/// <summary>
/// JSON 데이터를 담고 필드, 중첩 객체, 배열에 접근하는 클래스입니다.
/// 리소스 설정과 같은 순수 JSON 데이터에 사용합니다.
/// </summary>
class FJson
{
private:
	nlohmann::json Object;

public:
	FJson();
	explicit FJson(const nlohmann::json& InObject);

	nlohmann::json GetJSON() const { return Object; }

	int32 GetInt32(const FString& Key) const;
	void SetInt32(const FString& Key, int32 Value);

	float GetFloat(const FString& Key) const;
	void SetFloat(const FString& Key, float Value);

	uint32 GetUInt32(const FString& Key) const;
	void SetUInt32(const FString& Key, uint32 Value);

	double GetDouble(const FString& Key) const;
	void SetDouble(const FString& Key, double Value);

	bool GetBool(const FString& Key) const;
	void SetBool(const FString& Key, bool Value);

	FString GetString(const FString& Key) const;
	void SetString(const FString& Key, const FString& Value);

	FWString GetWString(const FString& Key) const;
	void SetWString(const FString& Key, const FWString& Value);

	bool IsNull(const FString& Key) const;
	void SetNull(const FString& Key);

	FVector GetVector(const FString& Key) const;
	void SetVector(const FString& Key, const FVector& Value);

	FVector2 GetVector2(const FString& Key) const;
	void SetVector2(const FString& Key, const FVector2& Value);

	FVector4 GetVector4(const FString& Key) const;
	void SetVector4(const FString& Key, const FVector4& Value);

	template <typename T>
	TArray<T> GetArray(const FString& Key) const;

	template <typename T>
	void SetArray(const FString& Key, const TArray<T>& Value);

	TArray<FJson> GetJsonArray(const FString& Key) const;
	void SetJsonArray(const FString& Key, const TArray<FJson>& Value);

	FJson GetJson(const FString& Key) const;
	void SetJson(const FString& Key, const FJson& Archive);

	template <typename T>
	T GetEnum(const FString& Key, TMap<FString, T>& EnumMap);

	template <typename T>
	void SetEnum(const FString& Key, T Value, TMap<T, FString>& EnumMap);
};

template <typename T>
inline TArray<T> FJson::GetArray(const FString& Key) const
{
	TArray<T> Array;

	for (const auto& Item : Object.at(Key))
	{
		T Value = Item.get<T>();
		Array.push_back(Value);
	}

	return Array;
}

template <typename T>
inline void FJson::SetArray(const FString& Key, const TArray<T>& Value)
{
	Object[Key] = nlohmann::json::array();

	for (int i = 0; i < Value.size(); ++i)
	{
		Object[Key].push_back(Value[i]);
	}
}

template <typename T>
inline T FJson::GetEnum(const FString& Key, TMap<FString, T>& EnumMap)
{
	return EnumMap.at(GetString(Key));
}

template <typename T>
inline void FJson::SetEnum(const FString& Key, T Value, TMap<T, FString>& EnumMap)
{
	SetString(Key, EnumMap.at(Value));
}
