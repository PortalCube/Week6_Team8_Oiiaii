#include "TestTextActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UTextComponent.h"

IMPLEMENT_UCLASS(ATestTextActor, AActor)
UCLASS_META(ATestTextActor, DisplayName, "Test Text Actor")

void ATestTextActor::Initialize()
{
	Super::Initialize();

	auto Component = CreateDefaultSubobject<UTextComponent>();
	if (Component)
	{
		SetRootComponent(Component);
	}
}
