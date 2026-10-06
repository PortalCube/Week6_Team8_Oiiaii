#include "ASpotlightActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

IMPLEMENT_UCLASS(ASpotlightActor, AActor)
UCLASS_META(ASpotlightActor, DisplayName, "Spotlight Actor")

void ASpotlightActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<USpotLightComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FTransform DefaultTransform;
		DefaultTransform.SetScale3D(FVector(5.0f, 5.0f, 5.0f));
		DefaultTransform.SetRotation(FQuaternion::FromEulerXYZDeg(FVector(0.0f, 90.0f, 0.0f)));
		SetTransform(DefaultTransform);
	}
}
