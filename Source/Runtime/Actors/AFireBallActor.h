#pragma once

#include "AActor.h"

class UStaticMeshComponent;
class UFireBallComponent;
class UProjectileMovementComponent;

class AFireBallActor : public AActor
{
	DECLARE_UCLASS(AFireBallActor, AActor)
	GENERATED_BODY()

public:
	explicit AFireBallActor();
	void Initialize() override;

	UStaticMeshComponent* GetSphereComponent() const;
	UFireBallComponent* GetFireBallComponent() const { return FireBallComponent; }

private:
	UFireBallComponent* FireBallComponent = nullptr;
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;
};
