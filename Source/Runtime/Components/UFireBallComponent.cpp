#include "UFireBallComponent.h"
#include "Runtime/CoreUObject/UClass.h"

IMPLEMENT_UCLASS(UFireBallComponent, UPrimitiveComponent)
UCLASS_META(UFireBallComponent, DisplayName, "FireBall")

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
