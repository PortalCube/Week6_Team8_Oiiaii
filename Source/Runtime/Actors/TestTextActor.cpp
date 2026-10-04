#include "TestTextActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UTextComponent.h"

IMPLEMENT_UCLASS(ATestTextActor, AActor)
UCLASS_META(ATestTextActor, DisplayName, "Test Text Actor")

void ATestTextActor::Initialize()
{
	// 텍스트 인스턴스 컴포넌트 장착
	CreateRootComponent(UTextComponent::StaticClass());
}

UTextComponent* ATestTextActor::GetTextInstanceComponent() const
{
	return RootComponent ? RootComponent->Cast<UTextComponent>() : nullptr;
}
