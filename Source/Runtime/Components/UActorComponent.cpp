
/*
#include "Runtime/CoreUObject/UClass.h"
#include "UActorComponent.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "UPrimitiveComponent.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/ULevel.h"

IMPLEMENT_UCLASS(UActorComponent, UObject)

void UActorComponent::Initialize()
{
	Super::Initialize();
	bHasBegunPlay = false;
	bTickEnabled = false;
}
void UActorComponent::Release()
{
	if (bHasBegunPlay)
	{
		EndPlay();
	}

	Unregister();

	Super::Release();
}

void UActorComponent::BeginPlay()
{
	if (bHasBegunPlay)
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
	if (bHasBegunPlay)
	{
		EndPlay();
	}
}

void UActorComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);
}

void UActorComponent::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);
}

*/
