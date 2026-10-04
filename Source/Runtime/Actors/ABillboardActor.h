#pragma once

#include "AActor.h"
#include "Runtime/Math/FVector.h"

class UBillboardComponent;

// 큐브 액터 정의
class ABillboardActor : public AActor
{
	DECLARE_UCLASS(ABillboardActor, AActor)
	GENERATED_BODY()

public:
	virtual void Initialize() override;
	UBillboardComponent* GetBillboardComponent() const;
};
