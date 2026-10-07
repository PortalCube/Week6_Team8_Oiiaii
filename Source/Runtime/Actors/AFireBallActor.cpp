#include "AFireBallActor.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"
#include "Runtime/Components/UFireBallComponent.h"
#include "Runtime/Components/UProjectileMovementComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Serialization/FArchive.h"

IMPLEMENT_UCLASS(AFireBallActor, AActor)
UCLASS_META(AFireBallActor, DisplayName, "FireBall Actor")

void AFireBallActor::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;

	// 루트 컴포넌트 생성 및 장착
	auto Component = CreateDefaultSubobject<UStaticMeshComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetMesh(Registry.Get<UStaticMesh>("#Sphere"));
		Component->SetColor(FVector4{ 1.0f, 0.0f, 0.0f, 1.0f });

		FireBallComponent = CreateDefaultSubobject<UFireBallComponent>();

		FireBallComponent->SetFireColor(FVector4{ 1.0f, 0.0f, 0.0f, 1.0f });
		FireBallComponent->SetEmissiveColor(FVector{ 1.0f, 0.0f, 0.0f });
		FireBallComponent->SetEmissiveIntensity(1.0f);
		FireBallComponent->SetIntensity(1.0f);
		FireBallComponent->SetRadius(10.0f);
		FireBallComponent->SetRadiusFalloff(2.0f);

		ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>();
	}
}

void AFireBallActor::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	if (FireBallComponent)
	{
		Archive.Reference("FireBallComponent", FireBallComponent);
	}
}

UStaticMeshComponent* AFireBallActor::GetSphereComponent() const
{
	return RootComponent ? RootComponent->Cast<UStaticMeshComponent>() : nullptr;
}
