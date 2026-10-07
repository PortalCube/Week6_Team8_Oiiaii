#pragma once

#include "AActor.h"
#include "Runtime/Components/UHeightFogComponent.h"

class AHeightFogActor : public AActor
{
	DECLARE_UCLASS(AHeightFogActor, AActor)
	GENERATED_BODY()

public:
	void Initialize() override;

};
