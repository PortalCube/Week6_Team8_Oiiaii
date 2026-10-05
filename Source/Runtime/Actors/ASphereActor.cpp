#include "ASphereActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ASphereActor, AActor)
UCLASS_META(ASphereActor, DisplayName, "Sphere Actor")

void ASphereActor::Initialize()
{
	// 기본 구체 컴포넌트 장착
	UStaticMeshComponent* Object = CreateDefaultSubobject<UStaticMeshComponent>();
	SetRootComponent(Object);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	Object->SetMesh(Registry.Get<UStaticMesh>("#Sphere"));
	Object->SetMaterial(Registry.Get<UMaterial>("Material/Textured.json"));
}
