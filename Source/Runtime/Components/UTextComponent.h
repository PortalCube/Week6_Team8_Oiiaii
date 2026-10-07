#pragma once

#include "Runtime/Components/UInstancePrimitiveComponent.h"
#include "Runtime/Rendering/FFont.h"
#include "Runtime/Rendering/FRenderQueue.h"

class FArchive;
class UFont;

class UTextComponent : public UInstancePrimitiveComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UTextComponent, UInstancePrimitiveComponent)

public:
	void Initialize() override;
	void TickComponent(float delta) override;

	void SetText(FWStringView InText);

	[[nodiscard]] const FWString& GetText() const { return Text; }
	// void SetFont(TSharedPtr<FFont> InFont);

	void SetFont(const FName& InName);
	void SetFont(UFont* InFont);
	UFont* GetFont() const { return FontAsset; }

	void SetTextColor(const FVector4& InColor);
	[[nodiscard]] const FVector4& GetTextColor() const { return TextColor; }

	void SetTextSize(float InSize);
	[[nodiscard]] float GetTextSize() const { return TextSize; }

	void RebuildTextMesh();

	// Object -> World 변환 행렬 생성
	virtual FMatrix GetRenderMatrix(const FCamera& Camera) const override;
	virtual const FRenderData& GetRenderData(const FCamera& Camera) const override;

	virtual EEngineShowFlags GetShowFlag() const
	{
		return EEngineShowFlags::SF_BillboardText;
	}

	virtual void Serialize(FArchive& Archive) override;


	float GetWidth() const { return Width; }
	float GetHeight() const { return Height; }

private:
	TSharedPtr<FFont> Font;
	UFont* FontAsset = nullptr;
	FWString Text = L"Hello Jungle World!";
	FVector4 TextColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float TextSize = 1.0f;

	float Width = 0.0f;
	float Height = 0.0f;

	TArray<FInstanceData> Instances;
};
