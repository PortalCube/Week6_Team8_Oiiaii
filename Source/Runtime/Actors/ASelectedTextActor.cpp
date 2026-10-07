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
	bIsEditorOnlyActor = true;

	TextComponent = CreateDefaultSubobject<UTextComponent>();
	if (TextComponent)
	{
		TextComponent->bIsEditorOnly = true;
		SetRootComponent(TextComponent);

		TextComponent->SetInheritRotation(false);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		TextComponent->SetFont(Registry.Get<UFont>("Font/NanumGothic.json"));
		TextComponent->SetText(L"");
	}
}
