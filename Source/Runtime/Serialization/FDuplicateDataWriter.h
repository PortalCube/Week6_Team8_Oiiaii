#pragma once

#include "Runtime/Serialization/FArchive.h"

class FDuplicateDataWriter : public FArchive
{
public:

	virtual void Serialize(UObject* Object) override;

	////////////////////////////////////////////////////////////
	// Field
	////////////////////////////////////////////////////////////

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

};
