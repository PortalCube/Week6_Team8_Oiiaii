#pragma once

#include "Runtime/Serialization/FArchive.h"

class FDuplicateDataReader : public FArchive
{
public:

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
};
