#pragma once

#include "AActor.h"

class UStaticMeshComponent;

class AAppleBittenActor : public AActor
{
	DECLARE_UCLASS(AAppleBittenActor, AActor)
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	virtual void Update(float DeltaTime) override;

private:
	UStaticMeshComponent* AppleStaticMeshComp;
};
