#pragma once

#include "UMovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UProjectileMovementComponent, UMovementComponent)

protected:
	UProjectileMovementComponent() = default;
};
