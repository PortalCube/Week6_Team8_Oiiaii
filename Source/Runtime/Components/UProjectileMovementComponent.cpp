#include "UProjectileMovementComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(UProjectileMovementComponent, UMovementComponent)
UCLASS_META(UProjectileMovementComponent, DisplayName, "Projectile Movement Component")

void UProjectileMovementComponent::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;
}

void UProjectileMovementComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	USceneComponent* Component = GetUpdatedComponent();
	if (!Component || DeltaTime <= 0.0f)
	{
		return;
	}

	Component->SetRelativeLocation(Component->GetRelativeLocation() + Velocity * DeltaTime);
}
