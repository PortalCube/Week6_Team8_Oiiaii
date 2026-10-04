#include "UWorld.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/UEngine.h"
#include "Runtime/Actors/AActor.h"

IMPLEMENT_UCLASS(UWorld, UObject)
UCLASS_META(UWorld, SerializeName, "World")

void UWorld::Initialize()
{
}

void UWorld::BeginPlay()
{
	// TODO: 모든 WorldSubsystem::BeginPlay 실행

	// TODO: GameModeBase::StartPlay 실행

	// 언리얼은 왠지 모르겠는데 이렇게 안함
	for (const auto& Actor : CurrentLevel->GetActors())
	{
		Actor->BeginPlay();
	}
}

void UWorld::Tick(float DeltaSeconds)
{
	// TODO: 언리얼은 실제로는 FTickTaskManager를 사용하여 틱을 처리
	for (const auto& Actor : CurrentLevel->GetActors())
	{
		if (Actor->GetTickEnabled())
		{
			Actor->Update(DeltaSeconds);
		}
	}

	CurrentLevel->UpdateDirtyBounds();
}

void UWorld::EndPlay()
{
	for (const auto& Actor : CurrentLevel->GetActors())
	{
		Actor->EndPlay();
	}
}

void UWorld::CleanupWorld()
{
	// 요소가 바뀌는 문제 때문에 while 문으로 실행
	while (!CurrentLevel->GetActors().empty())
	{
		DestroyActor(CurrentLevel->GetActors()[0]);
	}
}

void UWorld::Release()
{
	DestroyObject(CurrentLevel);
}

ULevel* UWorld::GetCurrentLevel() const
{
	return CurrentLevel;
}

void UWorld::LoadLevel(ULevel* Level)
{
	CurrentLevel = Level;
	Initialize();
	BeginPlay();
}

AActor* UWorld::SpawnActor(UClass* Class, FTransform const* Transform)
{
	if (!CurrentLevel)
	{
		return nullptr;
	}

	// 새로운 액터 생성
	AActor* Actor = NewObject<AActor>(this, Class);

	if (Transform)
	{
		Actor->SetTransform(*Transform);
	}

	// 생명주기 실행
	Actor->Initialize();
	Actor->Register(*CurrentLevel);
	Actor->BeginPlay();

	// 현재 레벨에 추가
	CurrentLevel->Actors.push_back(Actor);

	return Actor;

	return nullptr;
}

void UWorld::RemoveActor(AActor* Actor, bool bShouldModifyLevel)
{
	if (!CurrentLevel)
	{
		return;
	}

	TArray<AActor*>& Actors = CurrentLevel->Actors;

	for (int i = 0; i < Actors.size(); ++i)
	{
		if (Actors[i] == Actor)
		{
			// TODO: 매우 좋지 않은 방법
			// nullptr & swap 방식 생각해보기
			Actor->Unregister();
			Actors.erase(Actors.begin() + i);
		}
	}
}

UWorld* UWorld::GetWorld() const
{
	// It's me!

	// 언리얼 소스코드가 이래 되어있음
	// 소스코드의 주석에서는 const 한정자를 지우고 반환하는 것 까진 큰 문제가 되지 않는다고 설명.
	return const_cast<UWorld*>(this);
}

bool UWorld::DestroyActor(AActor* Actor)
{
	if (Actor == nullptr)
	{
		return false;
	}

	RemoveActor(Actor, false);
	DestroyObject(Actor);

	return true;
}
