#include "ASphereActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ASphereActor, AActor)
UCLASS_META(ASphereActor, DisplayName, "Sphere Actor")

void ASphereActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UStaticMeshComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetMesh(Registry.Get<UStaticMesh>("#Sphere"));
		Component->SetMaterial(Registry.Get<UMaterial>("Material/Textured.json"));
	}
}
