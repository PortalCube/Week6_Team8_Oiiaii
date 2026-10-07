#include "UFireBallComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Engine/FArchive.h"

IMPLEMENT_UCLASS(UFireBallComponent, UPrimitiveComponent)
UCLASS_META(UFireBallComponent, DisplayName, "FireBall")

void UFireBallComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);
	Archive.SetFloat("Intensity", Intensity);
	Archive.SetFloat("Radius", Radius);
	Archive.SetFloat("RadiusFalloff", RadiusFalloff);
	Archive.SetVector4("FireColor", FireColor);
	Archive.SetVector("EmissiveColor", EmissiveColor);
	Archive.SetFloat("EmissiveIntensity", EmissiveIntensity);
}

void UFireBallComponent::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	// 이전 씬에 없는 속성은 Initialize에서 설정한 값을 유지한다.
	if (!Archive.IsNull("Intensity"))
	{
		SetIntensity(Archive.GetFloat("Intensity"));
	}
	if (!Archive.IsNull("Radius"))
	{
		SetRadius(Archive.GetFloat("Radius"));
	}
	if (!Archive.IsNull("RadiusFalloff"))
	{
		SetRadiusFalloff(Archive.GetFloat("RadiusFalloff"));
	}
	if (!Archive.IsNull("FireColor"))
	{
		SetFireColor(Archive.GetVector4("FireColor"));
	}
	if (!Archive.IsNull("EmissiveColor"))
	{
		SetEmissiveColor(Archive.GetVector("EmissiveColor"));
	}
	if (!Archive.IsNull("EmissiveIntensity"))
	{
		SetEmissiveIntensity(Archive.GetFloat("EmissiveIntensity"));
	}
}

FPointLightConstants UFireBallComponent::GetPointLightData() const
{
	FPointLightConstants Data{};
	Data.PositionWS = GetGlobalTransform().GetLocation();
	Data.Radius = Radius;
	Data.Color = FVector{ FireColor.X, FireColor.Y, FireColor.Z };
	Data.Intensity = Intensity;
	Data.FalloffExponent = RadiusFalloff;
	return Data;
}
