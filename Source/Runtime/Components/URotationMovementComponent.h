#pragma once

#include "UMovementComponent.h"

class URotationMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(URotationMovementComponent, UMovementComponent)

public:
	void Initialize() override;
	void TickComponent(float DeltaTime) override;

	// Degrees per second around the updated component's local axes (XYZ).
	void SetRotationRate(const FVector& InRotationRate) { RotationRate = InRotationRate; }
	const FVector& GetRotationRate() const { return RotationRate; }

protected:
	FVector RotationRate{ 0.0f, 0.0f, 90.0f };
};
