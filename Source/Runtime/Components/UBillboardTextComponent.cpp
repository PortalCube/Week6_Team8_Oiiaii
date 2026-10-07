#include "UBillboardTextComponent.h"
#include "Runtime/Rendering/FBillboardRendering.h"
#include "Runtime/CoreUObject/UClass.h"

IMPLEMENT_UCLASS(UBillboardTextComponent, UTextComponent)
UCLASS_META(UBillboardTextComponent, DisplayName, "Billboard Text Component")

FMatrix UBillboardTextComponent::GetRenderMatrix(const FCamera& Camera) const
{
	FTransform Transform = GetGlobalTransform();

	FMatrix ScaleTransform = FMatrix::MakeScale({ 1.0f, GetWidth(), GetHeight() });
	FMatrix ModelMatrix = BillboardRendering::MakeBillboardMatrix(Transform, Camera);

	return ScaleTransform * ModelMatrix;
}

const FRenderData& UBillboardTextComponent::GetRenderData(const FCamera& Camera) const
{
	TArray<FInstanceData> Built;

	const auto& LocalInstances = GetLocalTextInstances();
	Built.reserve(LocalInstances.size());

	FTransform Transform = GetGlobalTransform();
	FMatrix ModelMatrix = BillboardRendering::MakeBillboardMatrix(Transform, Camera);

	// 글자별 FInstanceData에 빌보드 월드 행렬 적용
	for (const FInstanceData& Inst : LocalInstances)
	{
		FInstanceData WorldInst = Inst;
		WorldInst.World *= ModelMatrix;
		Built.push_back(WorldInst);
	}

	RenderData.Instances = std::move(Built);

	return RenderData;
}

void UBillboardTextComponent::UpdateWorldBounds()
{
	Super::UpdateWorldBounds();

	// 외접구 반지름
	float Radius = WorldBounds.Extent.Size();

	// AABB 계산 후 적용
	FVector Center = WorldBounds.Center;
	FVector Extent{ Radius, Radius, Radius };
	WorldBounds = { Center, Extent };
}
