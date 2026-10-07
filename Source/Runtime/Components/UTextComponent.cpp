#include "UTextComponent.h"
#include "Runtime/Rendering/FTextRendering.h"
#include "Runtime/Asset/UFont.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/CoreUObject/UClass.h"
#include <algorithm>
#include <limits>
#include <windows.h>

IMPLEMENT_UCLASS(UTextComponent, UInstancePrimitiveComponent)

void UTextComponent::Initialize()
{
	Super::Initialize();

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	SetMesh(Registry.Get<UStaticMesh>("#Rect"));
	SetMaterial(Registry.Get<UMaterial>("Material/Text.json"));
	SetFont(Registry.Get<UFont>("Font/BazziOTF.json"));

	RenderData.Type = ERenderType::Text;

	RebuildTextMesh();
}

void UTextComponent::TickComponent(float delta) {}

void UTextComponent::SetText(FWStringView InText)
{
	Text = InText;
	RebuildTextMesh();
}

// void UTextComponent::SetFont(TSharedPtr<FFont> InFont) {
//   Font = InFont;
//   RenderData.TextureId = FName("bazziotf");
//   RebuildTextMesh();
// }

void UTextComponent::SetFont(const FName& InName)
{
	Font = FRenderResourceLibrary::Get().GetFont(InName);
	FontAsset = nullptr;
	// RenderData.TextureId(InName);
	RebuildTextMesh();
}

void UTextComponent::SetFont(UFont* InFont)
{
	if (!InFont || !InFont->Get())
	{
		return;
	}

	FontAsset = InFont;
	Font = FRenderResourceLibrary::Get().GetFont(
	    std::filesystem::path(InFont->GetID().ToString()).stem().string());
	SetTexture(InFont->GetTexture());
	RebuildTextMesh();
}

void UTextComponent::SetTextColor(const FVector4& InColor)
{
	TextColor = InColor;

	for (FInstanceData& Instance : Instances)
	{
		Instance.Color = TextColor;
	}
}

void UTextComponent::SetTextSize(float InSize)
{
}

void UTextComponent::RebuildTextMesh()
{
	Instances.clear();
	Width = Height = 0.0f;
	if (Font)
	{
		TextRendering::BuildGlyphInstances(Text, *Font, TextColor, Instances, Width, Height);
	}
}
FMatrix UTextComponent::GetRenderMatrix(const FCamera& Camera) const
{
	FTransform Transform = GetGlobalTransform();

	FMatrix ScaleTransform = FMatrix::MakeScale({ 1.0f, Width, Height });
	FMatrix ModelMatrix = TextRendering::MakeBillboardMatrix(Transform, Camera);

	return ScaleTransform * ModelMatrix;
}

const FRenderData& UTextComponent::GetRenderData(const FCamera& Camera) const
{

	TArray<FInstanceData> Built;

	FTransform Transform = GetGlobalTransform();
	FMatrix ModelMatrix = TextRendering::MakeBillboardMatrix(Transform, Camera);

	// 글자별 FInstanceData에 빌보드 월드 행렬 적용
	for (const FInstanceData& Inst : Instances)
	{
		FInstanceData WorldInst = Inst;
		WorldInst.World *= ModelMatrix;
		Built.push_back(WorldInst);
	}

	RenderData.Instances = std::move(Built);

	return RenderData;
}

void UTextComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	Archive.SetWString("Text", Text);
	Archive.SetVector4("TextColor", TextColor);
	Archive.SetFloat("TextSize", TextSize);

	if (FontAsset)
	{
		Archive.SetString("FontAsset", FontAsset->GetID().ToString());
	}
}

void UTextComponent::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	Text = Archive.GetWString("Text");

	if (!Archive.IsNull("FontAsset"))
	{
		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		FString FontAssetID = Archive.GetString("FontAsset");
		UFont* LoadedFont = Registry.Get<UFont>(FontAssetID);

		if (LoadedFont)
		{
			SetFont(LoadedFont);
			return;
		}
	}

	RebuildTextMesh();
}
