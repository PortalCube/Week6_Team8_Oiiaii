#include "UFireBallComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Serialization/FArchive.h"

IMPLEMENT_UCLASS(UFireBallComponent, UPrimitiveComponent)
UCLASS_META(UFireBallComponent, DisplayName, "FireBall")

void UFireBallComponent::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	Archive.Field("Intensity", Intensity);
	Archive.Field("Radius", Radius);
	Archive.Field("RadiusFalloff", RadiusFalloff);
	Archive.Field("FireColor", FireColor);
	Archive.Field("EmissiveColor", EmissiveColor);
	Archive.Field("EmissiveIntensity", EmissiveIntensity);
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
