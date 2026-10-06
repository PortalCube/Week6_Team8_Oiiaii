#pragma once

#include "AActor.h"
#include "Runtime/Math/FVector.h"

class ASelectedTextActor : public AActor
{
	DECLARE_UCLASS(ASelectedTextActor, AActor)
	GENERATED_BODY()

public:
	virtual void Initialize() override;

	class UTextComponent* TextComponent = nullptr;

};
