#pragma once

#include "AActor.h"

class UStaticMeshComponent;
class UFireBallComponent;
class UProjectileMovementComponent;
class URotationMovementComponent;

class AFireBallActor : public AActor

{
	DECLARE_UCLASS(AFireBallActor, AActor)
	GENERATED_BODY()

public:
	void Initialize() override;
	void Serialize(FArchive& Archive) override;

	UStaticMeshComponent* GetSphereComponent() const;
	UFireBallComponent* GetFireBallComponent() const { return FireBallComponent; }
	URotationMovementComponent* GetRotationMovementComponent() const { return RotationMovementComponent; }
	UProjectileMovementComponent* GetProjectileMovementComponent() const { return ProjectileMovementComponent; }

private:
	UFireBallComponent* FireBallComponent = nullptr;
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;
	URotationMovementComponent* RotationMovementComponent = nullptr;
};
