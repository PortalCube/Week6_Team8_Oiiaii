#pragma once
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Math/FQuaternion.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "UPrimitiveComponent.h"

class ULevel;
class FArchive;

class UBillboardComponent : public UPrimitiveComponent
{
	DECLARE_UCLASS(UBillboardComponent, UPrimitiveComponent)
	GENERATED_BODY()

protected:
	virtual void Serialize(FArchive& Archive);

public:
	void Initialize() override;

	virtual void SetTexture(UTexture* Texture);
	UTexture* GetTexture() const;

	// Object -> World 변환 행렬 생성
	virtual FMatrix GetRenderMatrix(const FCamera& Camera) const override;
	virtual void UpdateWorldBounds() override;
	virtual EEngineShowFlags GetShowFlag() const { return EEngineShowFlags::SF_BillboardText; }

	void SetUVScale(FVector2 Value);
	void SetUVOffset(FVector2 Value);

	FVector2 GetUVScale() const;
	FVector2 GetUVOffset() const;

	virtual bool IsOcclusionTarget() const override { return false; }
};
