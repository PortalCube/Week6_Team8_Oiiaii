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
	FTransform Transform = GetGlobalTransform();

	FMatrix CameraRotation = Camera.GetRotationMatrix();
	FVector ViewForward = CameraRotation.TransformPointRow(FVector{ 1.0f, 0.0f, 0.0f }, 0.0f); // X+
	FVector ViewRight = CameraRotation.TransformPointRow(FVector{ 0.0f, 1.0f, 0.0f }, 0.0f);   // Y+
	FVector ViewUp = CameraRotation.TransformPointRow(FVector{ 0.0f, 0.0f, 1.0f }, 0.0f);      // Z+

	FVector Up = ViewUp * Transform.GetScale3D().Z;
	FVector Right = ViewRight * Transform.GetScale3D().Y;

	return FMatrix{
		FVector4{ ViewForward, 0.0f },
		FVector4{ Right, 0.0f },
		FVector4{ Up, 0.0f },
		FVector4{ Transform.GetLocation(), 1.0f },
	};
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
