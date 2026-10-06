#include "AFireBallActor.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Components/UFireBallComponent.h"
#include "Runtime/Components/UProjectileMovementComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AFireBallActor, AActor)
UCLASS_META(AFireBallActor, DisplayName, "FireBall Actor")

void AFireBallActor::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;

	UStaticMeshComponent* SphereComponent = CreateDefaultSubobject<UStaticMeshComponent>();
	SetRootComponent(SphereComponent);
	SphereComponent->SetMesh(FAssetRegistry::GetInstance().Get<UStaticMesh>("#Sphere"));
	SphereComponent->SetColor(FVector4{ 1.0f, 0.0f, 0.0f, 1.0f });

	FireBallComponent = CreateDefaultSubobject<UFireBallComponent>();
	AddComponent(FireBallComponent);
	FireBallComponent->SetFireColor(FVector4{ 1.0f, 0.0f, 0.0f, 1.0f });
	FireBallComponent->SetEmissiveColor(FVector{ 1.0f, 0.0f, 0.0f });
	FireBallComponent->SetEmissiveIntensity(1.0f);
	FireBallComponent->SetIntensity(1.0f);
	FireBallComponent->SetRadius(10.0f);
	FireBallComponent->SetRadiusFalloff(2.0f);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>();
	AddComponent(ProjectileMovementComponent);
}

UStaticMeshComponent* AFireBallActor::GetSphereComponent() const
{
	return RootComponent ? RootComponent->Cast<UStaticMeshComponent>() : nullptr;
}
