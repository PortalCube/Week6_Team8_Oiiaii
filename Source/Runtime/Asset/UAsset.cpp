#include "UAsset.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UAsset, UObject)

void UAsset::Initialize()
{
	Super::Initialize();

	// 외부 애셋으로 등록하여 얕은 복사 수행
	bIsExternal = true;
}

void UAsset::LoadInternal(UAssetDesc& Desc)
{
	ID = Desc.ID;
	Name = Desc.Name;
	AssetPath = Desc.AssetPath;
	AssetSize = Desc.AssetSize;
}
