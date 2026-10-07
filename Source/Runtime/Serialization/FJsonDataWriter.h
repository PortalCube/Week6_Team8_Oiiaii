#pragma once

#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Core/TStack.h"
#include "Runtime/Core/TQueue.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/Core/FLinearColor.h"

class FJsonDataWriter : public FArchive
{
public:

	FJsonDataWriter();
	// 출력 JSON과 쓰기 위치, 참조 상태를 초기화합니다.
	void Clear();
	FJsonDataWriter(const FJsonDataWriter&) = delete;
	FJsonDataWriter& operator=(const FJsonDataWriter&) = delete;
	using FArchive::Reference;
	nlohmann::json& GetJSON();
	nlohmann::json CloneJSON() const;

	virtual void Serialize(UObject* Object) override;

	////////////////////////////////////////////////////////////
	// Field
	////////////////////////////////////////////////////////////

	virtual void Reference(FStringView Key, UObject*& Value);

	virtual void Field(FStringView Key, UObject*& Value);

	virtual void Field(FStringView Key, int32& Value);
	virtual void Field(FStringView Key, int64& Value);

	virtual void Field(FStringView Key, uint32& Value);
	virtual void Field(FStringView Key, uint64& Value);

	virtual void Field(FStringView Key, float& Value);
	virtual void Field(FStringView Key, double& Value);
	virtual void Field(FStringView Key, bool& Value);

	virtual void Field(FStringView Key, FString& Value);
	virtual void Field(FStringView Key, FWString& Value);

	virtual void Field(FStringView Key, FVector& Value);
	virtual void Field(FStringView Key, FVector2& Value);
	virtual void Field(FStringView Key, FVector4& Value);

	virtual void Field(FStringView Key, FLinearColor& Value);

	template <typename T>
	void FieldInternal(FStringView Key, T Value);



	////////////////////////////////////////////////////////////
	// Section
	////////////////////////////////////////////////////////////

	virtual void BeginSection(FStringView Key);
	virtual void EndSection();



	////////////////////////////////////////////////////////////
	// Array
	////////////////////////////////////////////////////////////

	virtual int32 BeginArray(FStringView Key);
	virtual void EndArray();

private:

	nlohmann::json GetRefJson(int32 ReferenceKey);

	int32 AddReferenceKey(UObject* Reference);
	int32 GetReferenceKey(UObject* Reference);

	int32 GetNextArrayIndex();

	nlohmann::json* CurrentSection();
	nlohmann::json* GetCurrentNode(FStringView Key);

	nlohmann::json JSON;

	TQueue<UObject*> NextQueue;

	TStack<FArchiveSection> SectionStack;

	TMap<UObject*, int32> ReferenceTable;
};

template <typename T>
inline void FJsonDataWriter::FieldInternal(FStringView Key, T Value)
{
	*GetCurrentNode(Key) = Value;
}
