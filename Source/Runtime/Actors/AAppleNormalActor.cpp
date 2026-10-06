#include "AAppleNormalActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAppleNormalActor, AActor)
UCLASS_META(AAppleNormalActor, DisplayName, "Apple Normal Actor")

void AAppleNormalActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UStaticMeshComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/JungleApple/Apple_Normal.json"));
	}
}
