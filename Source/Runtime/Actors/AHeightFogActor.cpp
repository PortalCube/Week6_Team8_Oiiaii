#include "AHeightFogActor.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"


IMPLEMENT_UCLASS(AHeightFogActor, AActor)
UCLASS_META(AHeightFogActor, DisplayName, "HeightFog Actor")

void AHeightFogActor::Initialize()
{
	Super::Initialize();
	bTickEnabled = false;

	HeightFogComponent = CreateDefaultSubobject<UHeightFogComponent>();

	SetRootComponent(HeightFogComponent);

	FHeightFogSceneInfo Info{};

	HeightFogComponent->SetFogDensity(Info.FogDensity);
	HeightFogComponent->SetFogHeightFalloff(Info.FogHeightFalloff);
	HeightFogComponent->SetStartDistance(Info.StartDistance);
	HeightFogComponent->SetFogCutoffDistance(Info.FogCutoffDistance);
	HeightFogComponent->SetFogMaxOpacity(Info.FogMaxOpacity);
	HeightFogComponent->SetFogInscatteringColor(Info.FogInscatteringColor);

}

void AHeightFogActor::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	if (HeightFogComponent)
	{
		Archive.Reference("HeightFogComponent", HeightFogComponent);
	}
}
