#pragma once

#include "Runtime/Components/UTextComponent.h"

class UBillboardTextComponent : public UTextComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UBillboardTextComponent, UTextComponent)

public:

	FMatrix GetRenderMatrix(const FCamera& Camera) const override;

	const FRenderData& GetRenderData(const FCamera& Camera) const override;

	void UpdateWorldBounds() override;
};
