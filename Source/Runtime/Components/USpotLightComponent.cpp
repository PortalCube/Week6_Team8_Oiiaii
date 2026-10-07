#include "USpotLightComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/CoreUObject/UClass.h"

IMPLEMENT_UCLASS(USpotLightComponent, UPrimitiveComponent)
UCLASS_META(USpotLightComponent, DisplayName, "SpotLight")
UCLASS_META(USpotLightComponent, MeshName, "#SpotlightCone")

void USpotLightComponent::Initialize()
{
	Super::Initialize();
	// 스포트라이트 메쉬 및 머티리얼 장착
	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	SetMesh(Registry.Get<UStaticMesh>("#SpotlightCone"));
	SetMaterial(Registry.Get<UMaterial>("Material/Spotlight.json"));
	RenderData.Type = ERenderType::Spotlight;
}

void USpotLightComponent::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	Archive.Field("SpotAngle", SpotAngle);
	Archive.Field("Range", Range);
	Archive.Field("Intensity", Intensity);
	Archive.Field("LightColor", LightColor);
}
