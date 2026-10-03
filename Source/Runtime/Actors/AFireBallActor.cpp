#include "AFireBallActor.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Components/UFireBallComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AFireBallActor, AActor)
UCLASS_META(AFireBallActor, DisplayName, "FireBall Actor")

AFireBallActor::AFireBallActor()
{
	UStaticMeshComponent* SphereComponent = NewObject<UStaticMeshComponent>();
	SetRootComponent(SphereComponent);
	SphereComponent->SetMesh(FAssetRegistry::GetInstance().Get<UStaticMesh>("#Sphere"));

	FireBallComponent = NewObject<UFireBallComponent>();
	AddComponent(FireBallComponent);
}

UStaticMeshComponent* AFireBallActor::GetSphereComponent() const
{
	return RootComponent ? RootComponent->Cast<UStaticMeshComponent>() : nullptr;
}
