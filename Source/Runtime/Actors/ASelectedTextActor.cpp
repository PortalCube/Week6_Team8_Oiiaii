#include "ASelectedTextActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UTextComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ASelectedTextActor, AActor)
UCLASS_META(ASelectedTextActor, DisplayName, "Selected Text Actor")

void ASelectedTextActor::Initialize()
{
	Super::Initialize();

	TextComponent = CreateDefaultSubobject<UTextComponent>();
	if (TextComponent)
	{
		SetRootComponent(TextComponent);

		TextComponent->SetInheritRotation(false);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		TextComponent->SetFont(Registry.Get<UFont>("Font/BazziOTF.json"));
	}
}
