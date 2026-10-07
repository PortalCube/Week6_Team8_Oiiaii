#include "ACylinderActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ACylinderActor, AActor)
UCLASS_META(ACylinderActor, DisplayName, "Cylinder Actor")

void ACylinderActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UStaticMeshComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetMesh(Registry.Get<UStaticMesh>("#Cylinder"));
		Component->SetMaterial(Registry.Get<UMaterial>("Material/Textured.json"));
	}
}
