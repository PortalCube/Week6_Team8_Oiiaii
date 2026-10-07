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

void UActorComponent::Initialize()
{
	Super::Initialize();
}

void UActorComponent::Release()
{
	Super::Release();
}

void UActorComponent::RegisterComponent()
{
	OnRegister();
	// RegisterComponentTickFunction();
}

void UActorComponent::UnregisterComponent()
{
	OnUnregister();
}

void UActorComponent::OnRegister()
{
	if (bRegistered)
	{
		throw EngineUtil::CreateError("[UActorComponent::OnRegister] 컴포넌트가 이미 등록되었습니다.");
	}

	bRegistered = true;
}

void UActorComponent::BeginPlay()
{
	if (bHasBegunPlay)
	{
		throw EngineUtil::CreateError("[UActorComponent::BeginPlay] 이미 BeginPlay()가 실행되었습니다.");
	}

	bHasBegunPlay = true;
}

void UActorComponent::EndPlay()
{
	if (!bHasBegunPlay)
	{
		throw EngineUtil::CreateError("[UActorComponent::EndPlay] 이미 EndPlay()가 실행되었습니다.");
	}

	bHasBegunPlay = false;
}

void UActorComponent::OnUnregister()
{
	if (!bRegistered)
	{
		throw EngineUtil::CreateError("[UActorComponent::OnUnregister] 컴포넌트가 이미 등록 해제되었습니다.");
	}

	bRegistered = false;
}

bool UActorComponent::IsTickEnabled() const
{
	// 틱 비활성화
	if (!bTickEnabled)
	{
		return false;
	}

	// 에디터 틱 비활성화
	if (GetWorld()->GetWorldType() == EWorldType::Editor && !bTickInEditor)
	{
		return false;
	}

	return true;
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
