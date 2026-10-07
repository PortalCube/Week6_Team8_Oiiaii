#pragma once

#include "Runtime/Actors/AActor.h"

class UTextComponent;

// 텍스트 인스턴스 액터 선언
class ATestTextActor : public AActor
{
	DECLARE_UCLASS(ATestTextActor, AActor)
	GENERATED_BODY()

public:
	virtual void Initialize() override;
};
