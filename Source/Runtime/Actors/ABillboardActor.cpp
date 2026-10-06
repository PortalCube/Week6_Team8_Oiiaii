#include "ABillboardActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UBillboardComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ABillboardActor, AActor)
UCLASS_META(ABillboardActor, DisplayName, "Billboard Actor")

void ABillboardActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UBillboardComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetTexture(Registry.Get<UTexture>("Texture/Space.json"));
	}
}
