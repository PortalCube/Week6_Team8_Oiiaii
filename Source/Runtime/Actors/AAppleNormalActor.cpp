#include "AAppleNormalActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAppleNormalActor, AActor)
UCLASS_META(AAppleNormalActor, DisplayName, "Apple Normal Actor")

void AAppleNormalActor::Initialize()
{
	AppleStaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>();
	SetRootComponent(AppleStaticMeshComp);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	AppleStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/JungleApple/Apple_Normal.json"));
}

void AAppleNormalActor::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
}
