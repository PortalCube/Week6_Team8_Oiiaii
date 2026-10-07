#include "AActor.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/USceneComponent.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/UWorld.h"

IMPLEMENT_UCLASS(AActor, UObject)
UCLASS_META(AActor, DisplayName, "Actor")

void AActor::Initialize()
{
	Super::Initialize();
	bHasBegunPlay = false;
	bTickEnabled = false;
}

void AActor::Release()
{
	for (auto Component : OwnedComponents)
	{
		DestroyObject(Component);
	}

	RootComponent = nullptr;

	Super::Release();
}

void AActor::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);
	Archive.Reference("RootComponent", RootComponent);

	TArray<UActorComponent*> Components;
	for (auto* Component : OwnedComponents)
	{
		if (Component && !Component->IsEditorOnly())
			Components.push_back(Component);
	}
	if (Archive.IsReading())
	{
		if (RootComponent && std::find(OwnedComponents.begin(), OwnedComponents.end(), RootComponent) == OwnedComponents.end())
			OwnedComponents.push_back(RootComponent);
	}

	int32 Count = Archive.BeginArray("OwnedComponents");
	if (Archive.IsWriting()) Count = static_cast<int32>(Components.size());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UActorComponent* Component = Index < Components.size() ? Components[Index] : nullptr;
		Archive.Reference("", Component);
		if (Archive.IsReading() && Component && std::find(OwnedComponents.begin(), OwnedComponents.end(), Component) == OwnedComponents.end())
			OwnedComponents.push_back(Component);
	}
	Archive.EndArray();
}

ULevel* AActor::GetLevel() const
{
	return GetTypedOuter<ULevel>();
}

UWorld* AActor::GetWorld() const
{
	return GetLevel()->OwningWorld;
}

bool AActor::IsEditorOnly() const
{
	return bIsEditorOnlyActor;
}

void AActor::SetRootComponent(USceneComponent* Component)
{
	if (RootComponent)
	{
		// 지금은 SetRootComponent를 두번 실행하면 오류로 처리
		// 이런 기능이 필요하면 그때 처리 로직 추가
		throw EngineUtil::CreateError("[AActor::SetRootComponent] 이미 Root 컴포넌트가 있습니다.");
	}

	RootComponent = Component;

	// TODO: 이것저것

}

void AActor::MarkComponentsTransformDirty()
{
	for (auto Component : OwnedComponents)
	{
		USceneComponent* SceneComponent = Component->Cast<USceneComponent>();
		if (SceneComponent)
		{
			SceneComponent->OnTransformChanged();
		}
	}
}

void AActor::RegisterAllComponents()
{
	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->OnRegister();
		}
	}
}

void AActor::UnregisterAllComponents()
{
	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->OnUnregister();
		}
	}
}

void AActor::InitializeComponents()
{
	PreInitializeComponents();

	// 소유하는 모든 컴포넌트 초기화
	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->InitializeComponent();
		}
	}

	PostInitializeComponents();
}

void AActor::UninitializeComponents()
{
	// 소유하는 모든 컴포넌트 초기화 해제
	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->UninitializeComponent();
		}
	}
}

void AActor::PostSpawnInitialize()
{
	// 액터를 월드에 등록
	RegisterAllComponents();

	// 액터 생성 후 이벤트를 실행
	PostActorCreated();

	// 액터의 컴포넌트 초기화
	InitializeComponents();

	// FinishSpawning -> PostActorConstruction -> DispatchBeginPlay -> BeginPlay
	// 액터를 시작
	BeginPlay();
}


void AActor::BeginPlay()
{
	if (bHasBegunPlay)
	{
		return;
	}

	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->BeginPlay();
		}
	}

	bHasBegunPlay = true;
}

void AActor::Tick(float DeltaTime)
{
	if (!bTickEnabled || !bHasBegunPlay)
	{
		return;
	}

	for (auto Component : OwnedComponents)
	{
		if (Component && Component->IsTickEnabled())
		{
			Component->TickComponent(DeltaTime);
		}
	}
}

void AActor::EndPlay()
{
	if (!bHasBegunPlay)
	{
		return;
	}

	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->EndPlay();
		}
	}

	bHasBegunPlay = false;
}

bool AActor::GetTickEnabled() const
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

void AActor::Destroy()
{
	GetWorld()->DestroyActor(this);
}
