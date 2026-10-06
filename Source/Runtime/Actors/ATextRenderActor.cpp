#include "ATextRenderActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UTextComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(ATextRenderActor, AActor)
UCLASS_META(ATextRenderActor, DisplayName, "TextRender Actor")

void ATextRenderActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UTextComponent>();
	if (Component)
	{
		SetRootComponent(Component);

		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		Component->SetFont(Registry.Get<UFont>("Font/BazziOTF.json"));
	}
}
