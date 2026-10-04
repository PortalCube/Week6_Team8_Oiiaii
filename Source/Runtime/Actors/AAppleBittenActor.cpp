#include "AAppleBittenActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAppleBittenActor, AActor)
UCLASS_META(AAppleBittenActor, DisplayName, "Apple Bitten Actor")

void AAppleBittenActor::Initialize()
{
	AppleStaticMeshComp = NewObject<UStaticMeshComponent>(this);
	SetRootComponent(AppleStaticMeshComp);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	AppleStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/JungleApple/Apple_Bitten.json"));
}

void AAppleBittenActor::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
}
