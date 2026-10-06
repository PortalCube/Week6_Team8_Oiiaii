#include "AAppleBittenActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAppleBittenActor, AActor)
UCLASS_META(AAppleBittenActor, DisplayName, "Apple Bitten Actor")

void AAppleBittenActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UStaticMeshComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/JungleApple/Apple_Bitten.json"));
	}
}
