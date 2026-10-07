#pragma once

#include "Runtime/CoreUObject/UObject.h"

#include "Runtime/Core/FName.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"

struct UAssetDesc
{
	FName ID = "";
	FName Name = "";
	FString AssetPath = "";
	uint64 AssetSize = 0;
};

class UAsset : public UObject
{

	GENERATED_BODY()
	DECLARE_UCLASS(UAsset, UObject)

protected:
	FName ID = "";
	FName Name = "";
	FString AssetPath = "";
	uint64 AssetSize = 0;

	virtual void Initialize() override;
	void LoadInternal(UAssetDesc& Desc);

public:
	const FName& GetID() const { return ID; }
	FString GetIDString() const { return ID.ToString(); }
	const FName& GetName() const { return Name; }
};
