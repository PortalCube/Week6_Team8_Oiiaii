#pragma once

#include "USceneComponent.h"

class UMovementComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UMovementComponent, USceneComponent)

protected:
	UMovementComponent() = default;
};
