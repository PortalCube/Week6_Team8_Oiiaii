#include "UMovementComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Actors/AActor.h"

IMPLEMENT_UCLASS(UMovementComponent, USceneComponent)
UCLASS_META(UMovementComponent, DisplayName, "Movement")

void UMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!GetUpdatedComponent() && GetOwner())
	{
		SetUpdatedComponent(GetOwner()->GetRootComponent());
	}
}

void UMovementComponent::SetUpdatedComponent(USceneComponent* Component)
{
	// Movement components update a scene component owned by the same actor.
	if (Component && (Component == this || Component->GetOwner() != GetOwner()))
	{
		return;
	}
	UpdatedComponent = Component;
}
