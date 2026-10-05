#pragma once

#include "UMovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UProjectileMovementComponent, UMovementComponent)

public:
	void Initialize() override;

protected:
	UProjectileMovementComponent() = default;

	FVector Velocity{ 100.0f, 0.0f, 0.0f };
};
