#pragma once

#include "USceneComponent.h"
#include "Runtime/CoreUObject/TWeakObjectPtr.h"

class UMovementComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UMovementComponent, USceneComponent)

public:
	void BeginPlay() override;
	void SetUpdatedComponent(USceneComponent* Component);
	USceneComponent* GetUpdatedComponent() const { return UpdatedComponent.Get(); }

protected:
	TWeakObjectPtr<USceneComponent> UpdatedComponent;
};
