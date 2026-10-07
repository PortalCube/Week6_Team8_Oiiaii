#pragma once

#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Core/TStack.h"
#include "Runtime/Core/TQueue.h"
#include "ThirdParty/Json/json.hpp"

class FJsonDataWriter : public FArchive
{
public:

	FJsonDataWriter();

	nlohmann::json& GetJSON();
	nlohmann::json CloneJSON() const;

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

	template <typename T>
	void FieldInternal(FStringView Key, T Value);



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
