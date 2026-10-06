#include "AActor.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/USceneComponent.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/UWorld.h"

IMPLEMENT_UCLASS(AActor, UObject)

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

void AActor::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	if (RootComponent)
	{
		FArchive RootArchive{};
		RootComponent->Serialize(RootArchive);
		Archive.SetArchive("RootComponent", RootArchive);
	}
	else
	{
		Archive.SetNull("RootComponent");
	}
}

void AActor::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	if (Archive.IsNull("RootComponent"))
	{
		if (RootComponent)
		{
			UE_LOG_WARN("[%s::Deserialize] RootComponent(%s)에 대한 직렬화 데이터가 누락되었습니다.",
			    GetClass()->GetName(),
			    RootComponent->GetClass()->GetName());
		}
		return;
	}

	FArchive RootComponentArchive = Archive.GetArchive("RootComponent");
	const FString& SavedTypeName = RootComponentArchive.GetString("Type");
	UClass* SavedClass = UClass::FindByName(SavedTypeName);

	if (SavedClass == nullptr)
	{
		UE_LOG_WARN("[%s::Deserialize] 알 수 없는 타입 %s",
		    GetClass()->GetName(), SavedTypeName);
		return;
	}

	if (RootComponent == nullptr)
	{
		CreateRootComponent(SavedClass);

		if (RootComponent == nullptr)
		{
			UE_LOG_WARN("[%s::Deserialize] RootComponent %s를 생성할 수 없습니다.",
			    GetClass()->GetName(), SavedTypeName);
			return;
		}
	}

	if (RootComponent->GetClass() != SavedClass)
	{
		UE_LOG_WARN("[%s::Deserialize] 기본 RootComponent (%s)와 저장된 타입 "
		            "(%s)가 일치하지 않습니다.",
		    GetClass()->GetName(),
		    RootComponent->GetClass()->GetName(), SavedTypeName);
		return;
	}

	RootComponent->Deserialize(RootComponentArchive);
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

void AActor::CreateRootComponent(UClass* ClassType)
{
	if (RootComponent)
	{
		return;
	}

	USceneComponent* Component = NewObject<USceneComponent>(this, ClassType);
	SetRootComponent(Component);
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

void AActor::Destroy()
{
	GetWorld()->DestroyActor(this);
}
