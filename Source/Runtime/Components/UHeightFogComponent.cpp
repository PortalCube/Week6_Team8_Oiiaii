#include "UHeightFogComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Serialization/FArchive.h"

IMPLEMENT_UCLASS(UHeightFogComponent, UPrimitiveComponent)
UCLASS_META(UHeightFogComponent, DisplayName, "Fog")

void UHeightFogComponent::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	Archive.Field("FogDensity", Info.FogDensity);
	Archive.Field("FogHeightFalloff", Info.FogHeightFalloff);
	Archive.Field("StartDistance", Info.StartDistance);
	Archive.Field("FogCutoffDistance", Info.FogCutoffDistance);
	Archive.Field("FogMaxOpacity", Info.FogMaxOpacity);
	Archive.Field("FogInscatteringColor", Info.FogInscatteringColor);
}
