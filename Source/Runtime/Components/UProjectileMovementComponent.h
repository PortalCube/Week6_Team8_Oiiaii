#pragma once

#include "UMovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UProjectileMovementComponent, UMovementComponent)

public:
	void Initialize() override;
	void TickComponent(float DeltaTime) override;

	// Units per second in the updated component's parent space.
	void SetVelocity(const FVector& InVelocity) { Velocity = InVelocity; }
	const FVector& GetVelocity() const { return Velocity; }

protected:
	FVector Velocity{ 100.0f, 0.0f, 0.0f };
};
