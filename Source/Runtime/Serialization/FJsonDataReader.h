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

	void Clear();

	virtual void Serialize(UObject* Object) override;

	////////////////////////////////////////////////////////////
	// Field
	////////////////////////////////////////////////////////////

	using FArchive::Reference;
	virtual void Reference(FStringView Key, UObject*& Value) override;

	virtual void Field(FStringView Key, UObject*& Value) override;

	virtual void Field(FStringView Key, int32& Value) override;
	virtual void Field(FStringView Key, int64& Value) override;

	virtual void Field(FStringView Key, uint32& Value) override;
	virtual void Field(FStringView Key, uint64& Value) override;

	virtual void Field(FStringView Key, float& Value) override;
	virtual void Field(FStringView Key, double& Value) override;
	virtual void Field(FStringView Key, bool& Value) override;

	virtual void Field(FStringView Key, FString& Value) override;
	virtual void Field(FStringView Key, FWString& Value) override;

	virtual void Field(FStringView Key, FVector& Value) override;
	virtual void Field(FStringView Key, FVector2& Value) override;
	virtual void Field(FStringView Key, FVector4& Value) override;

	virtual void Field(FStringView Key, FLinearColor& Value);

	template <typename T>
	void FieldInternal(FStringView Key, T& Value);



	////////////////////////////////////////////////////////////
	// Section
	////////////////////////////////////////////////////////////

	virtual void BeginSection(FStringView Key) override;
	virtual void EndSection() override;



	////////////////////////////////////////////////////////////
	// Array
	////////////////////////////////////////////////////////////

	virtual int32 BeginArray(FStringView Key) override;
	virtual void EndArray() override;

private:

	int32 GetRefKey(const nlohmann::json& Node);

	UObject* AddReferenceKey(int32 Reference, UObject* Existing = nullptr);
	UObject* GetReferenceKey(int32 Reference);

	FArchiveSection* CurrentSection();
	nlohmann::json* GetCurrentNode(FStringView Key);

	nlohmann::json JSON;

	TQueue<int32> NextQueue;

	TStack<FArchiveSection> SectionStack;

	TMap<int32, UObject*> ReferenceTable;

};

template <typename T>
inline void FJsonDataReader::FieldInternal(FStringView Key, T& Value)
{
	const nlohmann::json* Node = GetCurrentNode(Key);

	if (Node && !Node->is_null())
	{
		Value = Node->get<T>();
	}
}
