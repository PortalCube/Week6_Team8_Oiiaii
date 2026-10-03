#pragma once

#include "AActor.h"

class UStaticMeshComponent;
class UFireBallComponent;

class AFireBallActor : public AActor
{
	DECLARE_UCLASS(AFireBallActor, AActor)
	GENERATED_BODY()

public:
	explicit AFireBallActor();

	UStaticMeshComponent* GetSphereComponent() const;
	UFireBallComponent* GetFireBallComponent() const { return FireBallComponent; }

private:
	UFireBallComponent* FireBallComponent = nullptr;
};
