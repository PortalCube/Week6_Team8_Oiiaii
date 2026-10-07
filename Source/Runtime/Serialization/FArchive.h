#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Engine/Types/EngineTypes.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Math/FVector4.h"
#include "Runtime/Core/FLinearColor.h"

#include "Runtime/Utility/EngineUtil.h"

/// <summary>
/// UObject의 데이터를 직렬화/역직렬화 하는 클래스입니다.
/// UObject의 데이터를 이 클래스에 담을 수도 있고, 이 데이터로 UObject를 만들 수도 있습니다.
/// </summary>
class FArchive
{
public:
	bool IsReading() const { return Direction == EArchiveDirection::Read; }
	bool IsWriting() const { return Direction == EArchiveDirection::Write; }
	virtual void Serialize(UObject* Object) = 0;

	////////////////////////////////////////////////////////////
	// Field
	////////////////////////////////////////////////////////////

	virtual void Reference(FStringView Key, UObject*& Value) = 0;

	// 파생 객체 포인터도 동일한 참조 테이블을 사용합니다.
	template <typename T>
	void Reference(FStringView Key, T*& Value)
	{
		UObject* Object = Value;
		Reference(Key, Object);
		if (IsReading())
		{
			Value = static_cast<T*>(Object);
		}
	}

	virtual void Field(FStringView Key, UObject*& Value) = 0;

	virtual void Field(FStringView Key, int32& Value) = 0;
	virtual void Field(FStringView Key, int64& Value) = 0;

	virtual void Field(FStringView Key, uint32& Value) = 0;
	virtual void Field(FStringView Key, uint64& Value) = 0;

	virtual void Field(FStringView Key, float& Value) = 0;
	virtual void Field(FStringView Key, double& Value) = 0;
	virtual void Field(FStringView Key, bool& Value) = 0;

	virtual void Field(FStringView Key, FString& Value) = 0;
	virtual void Field(FStringView Key, FWString& Value) = 0;

	virtual void Field(FStringView Key, FVector& Value) = 0;
	virtual void Field(FStringView Key, FVector2& Value) = 0;
	virtual void Field(FStringView Key, FVector4& Value) = 0;

	virtual void Field(FStringView Key, FLinearColor& Value) = 0;


	////////////////////////////////////////////////////////////
	// Section
	////////////////////////////////////////////////////////////

	virtual void BeginSection(FStringView Key) = 0;
	virtual void EndSection() = 0;



	////////////////////////////////////////////////////////////
	// Array
	////////////////////////////////////////////////////////////

	// 직렬화 과정에서는 0을 반환
	// 역직렬화 과정에서는 배열의 원소 갯수를 반환
	virtual int32 BeginArray(FStringView Key) = 0;
	virtual void EndArray() = 0;


	virtual ~FArchive() = default;

protected:

	EArchiveDirection Direction = EArchiveDirection::None;
	EArchiveMode Mode = EArchiveMode::Persistence;

};
