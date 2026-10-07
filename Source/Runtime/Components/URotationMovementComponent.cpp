#include "URotationMovementComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(URotationMovementComponent, UMovementComponent)
UCLASS_META(URotationMovementComponent, DisplayName, "Rotation Movement")

void URotationMovementComponent::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;
}

void URotationMovementComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);
	USceneComponent* Component = GetUpdatedComponent();
	if (!Component || DeltaTime <= 0.0f)
	{
		return;
	}

	const FQuaternion DeltaRotation = FQuaternion::FromEulerXYZDeg(RotationRate * DeltaTime);
	Component->SetRelativeRotation((Component->GetRelativeRotation() * DeltaRotation).Normalized());
}
