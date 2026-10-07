#include "ACubeActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ACubeActor, AActor)
UCLASS_META(ACubeActor, DisplayName, "Cube Actor")

void ACubeActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UStaticMeshComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetMesh(Registry.Get<UStaticMesh>("#Cube"));
	    Component->SetMaterial(Registry.Get<UMaterial>("Material/Cube_TwoSided.json"));

		// 테스트
		auto Component2 = CreateDefaultSubobject<UStaticMeshComponent>();
		if (Component2)
		{
			Component2->SetupAttachment(Component);

			FAssetRegistry& Registry = FAssetRegistry::GetInstance();
			Component2->SetMesh(Registry.Get<UStaticMesh>("#Cube"));
			Component2->SetMaterial(Registry.Get<UMaterial>("Material/Cube_TwoSided.json"));

			// 테스트
		}
	}
}
