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
	Unregister();

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
		{
			Components.push_back(Component);
		}
	}

	if (Archive.IsReading())
	{
		if (RootComponent)
		{
			auto It = std::find(OwnedComponents.begin(), OwnedComponents.end(), RootComponent);

			if (It == OwnedComponents.end())
			{
				OwnedComponents.push_back(RootComponent);
			}
		}
	}

	int32 Count = Archive.BeginArray("OwnedComponents");
	{
		if (Archive.IsWriting())
		{
			Count = static_cast<int32>(Components.size());
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			UActorComponent* Component = Index < Components.size() ? Components[Index] : nullptr;
			Archive.Reference("", Component);

			if (Archive.IsReading())
			{
				if (!Component)
				{
					continue;
				}

				auto It = std::find(OwnedComponents.begin(), OwnedComponents.end(), Component);

				if (It == OwnedComponents.end())
				{
					OwnedComponents.push_back(Component);
				}
			}
		}
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
	RootComponent = Component;

	// TODO: 이것저것

}

UActorComponent* AActor::AddComponent(UClass* Class)
{
	UActorComponent* Component = NewObject<UActorComponent>(this, Class);

	// 컴포넌트 등록
	OwnedComponents.push_back(Component);

	Component->Initialize();
	Component->Register();
	if (bHasBegunPlay)
	{
		Component->BeginPlay();
	}

	return Component;
}

void AActor::RemoveComponent(UActorComponent* Component)
{
	if (!Component)
	{
		return;
	}

	// 루트 컴포넌트는 삭제 불가
	if (Component == RootComponent)
	{
		return;
	}

	USceneComponent* SceneComponent = Component->Cast<USceneComponent>();
	if (SceneComponent)
	{
		// 자식 트리구조 정리
		TArray<USceneComponent*> Children = SceneComponent->Children;

		// 부모
		USceneComponent* Parent = SceneComponent->AttachParent;

		// TODO: 부모가 없는 경우는 일단 삭제 취소
		if (!Parent)
		{
			return;
		}

		FTransform& CurrentTransform = SceneComponent->RelativeTransform;

		for (auto Item : Children)
		{
			// Transform 수정
			FTransform NewTransform = Item->GetRelativeTransform();
			NewTransform = CurrentTransform * NewTransform;
			Item->SetRelativeTransform(NewTransform);

			// 새로운 부모로 이동
			Item->AttachToComponent(Parent);
		}

		SceneComponent->DetachFromComponent();
	}

	// 컴포넌트 삭제
	Component->EndPlay();
	Component->Unregister();
	std::erase(OwnedComponents, Component);
	DestroyObject(Component);

}

FTransform AActor::GetTransform() const
{
	return RootComponent ? RootComponent->GetRelativeTransform() : FTransform{};
}

void AActor::SetTransform(const FTransform& NewTransform)
{
	if (RootComponent)
	{
		RootComponent->SetRelativeTransform(NewTransform);
	}
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

void AActor::Register()
{
	if (bRegistered)
	{
		return;
	}

	if (!RootComponent)
	{
		SetRootComponent(CreateDefaultSubobject<USceneComponent>());
	}

	bRegistered = true;

	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->Register();
		}
	}
}

void AActor::Unregister()
{
	EndPlay();

	if (!bRegistered)
	{
		return;
	}

	bRegistered = false;

	for (auto Component : OwnedComponents)
	{
		if (Component)
		{
			Component->Unregister();
		}
	}
}

void AActor::BeginPlay()
{
	if (!bRegistered || bHasBegunPlay)
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
	if (!GetTickEnabled())
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

void AActor::Destroy()
{
	GetWorld()->DestroyActor(this);
}
