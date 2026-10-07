#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Utility/EngineUtil.h"
#include "ThirdParty/Json/json.hpp"
#include "UActorComponent.h"
#include "UPrimitiveComponent.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Engine/UWorld.h"
#include "Runtime/Engine/ULevel.h"

IMPLEMENT_UCLASS(UActorComponent, UObject)
UCLASS_META(UActorComponent, DisplayName, "Actor Component")

void UActorComponent::Initialize()
{
	Super::Initialize();
}

void UActorComponent::Release()
{
	Unregister();

	Super::Release();
}

void UActorComponent::Register()
{
	if (bRegistered)
	{
		return;
	}

	bRegistered = true;
}

void UActorComponent::BeginPlay()
{
	if (!bRegistered || bHasBegunPlay)
	{
		return;
	}

	bHasBegunPlay = true;
}

void UActorComponent::EndPlay()
{
	if (!bHasBegunPlay)
	{
		return;
	}

	bHasBegunPlay = false;
}

void UActorComponent::Unregister()
{
	EndPlay();

	if (!bRegistered)
	{
		return;
	}

	bRegistered = false;
}

bool UActorComponent::IsTickEnabled() const
{
	// 틱 비활성화
	if (!bRegistered || !bTickEnabled)
	{
		return false;
	}

	// 에디터 틱 비활성화
	if (GetWorld()->GetWorldType() == EWorldType::Editor && !bTickInEditor)
	{
		return false;
	}

	return bHasBegunPlay || GetWorld()->GetWorldType() == EWorldType::Editor;
}

void UActorComponent::MarkAsEditorOnlySubobject()
{
	bIsEditorOnly = true;
	bIsVisualizationComponent = true;
}

AActor* UActorComponent::GetOwner() const
{
	return GetTypedOuter<AActor>();
}

ULevel* UActorComponent::GetComponentLevel() const
{
	AActor* MyOwner = GetOwner();

	if (MyOwner)
	{
		return MyOwner->GetLevel();
	}
	else
	{
		return GetTypedOuter<ULevel>();
	}
}

UWorld* UActorComponent::GetWorld() const
{
	return GetComponentLevel()->GetWorld();
}

bool UActorComponent::IsEditorOnly() const
{
	return bIsEditorOnly;
}

void UActorComponent::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);
}
