#include "Runtime/Rendering/FBillboardRendering.h"
#include "UBillboardComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Asset/UTexture.h"
#include "Runtime/CoreUObject/UClass.h"
#include <algorithm>
#include <cctype>

IMPLEMENT_UCLASS(UBillboardComponent, UPrimitiveComponent)
UCLASS_META(UBillboardComponent, DisplayName, "BillBoard")
UCLASS_META(UBillboardComponent, MeshName, "BillBoard")

void UBillboardComponent::Initialize()
{
	Super::Initialize();

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	SetMesh(Registry.Get<UStaticMesh>("#Rect"));
	SetMaterial(Registry.Get<UMaterial>("Material/Billboard.json"));

	RenderData.Type = ERenderType::Primitive;
}

void UBillboardComponent::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);
	UTexture* Texture = GetTexture();
	FString TextureAssetID = Texture ? Texture->GetIDString() : "";
	Archive.Field("TextureAsset", TextureAssetID);

	if (Archive.IsReading() && !TextureAssetID.empty())
	{
		UTexture* LoadedTexture = FAssetRegistry::GetInstance().Get<UTexture>(TextureAssetID);

		if (LoadedTexture)
		{
			SetTexture(LoadedTexture);
		}
	}
}

void UBillboardComponent::SetTexture(UTexture* Texture)
{
	UPrimitiveComponent::SetTexture(Texture);
}

UTexture* UBillboardComponent::GetTexture() const
{
	return RenderData.Materials.empty() ? nullptr : RenderData.Materials[0].Texture;
}

FMatrix UBillboardComponent::GetRenderMatrix(const FCamera& Camera) const
{
	return BillboardRendering::MakeBillboardMatrix(GetGlobalTransform(), Camera);
}

void UBillboardComponent::UpdateWorldBounds()
{
	// 1. 사각형에 Transform 적용
	WorldBounds = { GetLocalBounds(), GetGlobalTransformMatrix() };

	// 2. 외접구 반지름
	float Radius = WorldBounds.Extent.Size();

	// 3. AABB 계산 후 적용
	FVector Center = WorldBounds.Center;
	FVector Extent{ Radius, Radius, Radius };
	WorldBounds = { Center, Extent };
}

void UBillboardComponent::SetUVScale(FVector2 Value)
{
	RenderData.Materials[0].UVScale = Value;
}

void UBillboardComponent::SetUVOffset(FVector2 Value)
{
	RenderData.Materials[0].UVOffset = Value;
}

FVector2 UBillboardComponent::GetUVScale() const
{
	return RenderData.Materials[0].UVScale;
}

FVector2 UBillboardComponent::GetUVOffset() const
{
	return RenderData.Materials[0].UVOffset;
}
