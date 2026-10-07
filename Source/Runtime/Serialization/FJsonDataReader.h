#pragma once

#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Core/TStack.h"
#include "Runtime/Core/TQueue.h"
#include "ThirdParty/Json/json.hpp"

class FJsonDataReader : public FArchive
{
public:

	FJsonDataReader(const nlohmann::json& InJSON);
	void SetJSON(const nlohmann::json& InJSON);
	// 입력 JSON은 유지하고 읽기 위치와 참조 상태를 초기화합니다.
	void Clear();
	FJsonDataReader(const FJsonDataReader&) = delete;
	FJsonDataReader& operator=(const FJsonDataReader&) = delete;
	using FArchive::Reference;

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
	void FieldInternal(FStringView Key, T& Value);



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

	int32 GetRefKey(const nlohmann::json& Node);

	UObject* AddReferenceKey(int32 Reference, UObject* Existing = nullptr);
	UObject* GetReferenceKey(int32 Reference);

	FArchiveSection* CurrentSection();
	nlohmann::json* GetCurrentNode(FStringView Key);
	UObject* CurrentObject();

	nlohmann::json JSON;

	TQueue<int32> NextQueue;

	TStack<FArchiveSection> SectionStack;

	TMap<int32, UObject*> ReferenceTable;

};

template <typename T>
inline void FJsonDataReader::FieldInternal(FStringView Key, T& Value)
{
	const nlohmann::json* Node = GetCurrentNode(Key);
	if (Node && !Node->is_null()) Value = Node->get<T>();
}
