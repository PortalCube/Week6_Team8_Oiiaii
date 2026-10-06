#include "UWorld.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/UEngine.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Actors/AActor.h"

IMPLEMENT_UCLASS(UWorld, UObject)
UCLASS_META(UWorld, SerializeName, "World")

void UWorld::Initialize()
{
}

void UWorld::InitWorld()
{
	// TODO: 물리, 네비게이션 시스템, 라인 트레이스, BVH 초기화...

	// PersistentLevel 등록
	Levels.push_back(PersistentLevel);

}

void UWorld::BeginPlay()
{
	// TODO: 모든 WorldSubsystem::BeginPlay 실행

	// TODO: GameModeBase::StartPlay 실행

	// 모든 레벨 순회하면서 BeginPlay() 실행
	for (auto Level : Levels)
	{
		if (Level)
		{
			Level->BeginPlay();
		}
	}
}

void UWorld::Tick(float DeltaSeconds)
{
	// TODO: 언리얼은 실제로는 FTickTaskManager를 사용하여 틱을 처리
	for (auto Level : Levels)
	{
		if (Level)
		{
			Level->Tick(DeltaSeconds);
		}
	}

	CurrentLevel->UpdateDirtyBounds();
}

void UWorld::EndPlay()
{
	// 모든 액터 순회하면서 EndPlay() 실행
	for (auto Level : Levels)
	{
		if (Level)
		{
			Level->EndPlay();
		}
	}
}

void UWorld::CleanupWorld()
{
	ClearWorldComponents();

	for (auto Level : Levels)
	{
		if (Level)
		{
			Level->CleanupLevel();
		}
	}
}

void UWorld::Release()
{
	for (auto Level : Levels)
	{
		if (Level)
		{
			DestroyObject(Level);
		}
	}
}

void UWorld::UpdateWorldComponents()
{
	// 모든 액터, 컴포넌트를 서브 시스템에 등록.
	// BVH 빌드도 여기서 실행
	for (ULevel* Level : Levels)
	{
		if (Level)
		{
			Level->UpdateLevelComponents();
		}
	}
}

void UWorld::ClearWorldComponents()
{
	for (ULevel* Level : Levels)
	{
		Level->ClearLevelComponents();
	}
}

void UWorld::InitializeActorsForPlay()
{
	// 모든 액터, 컴포넌트의 BeginPlay 이전 초기화
	for (ULevel* Level : Levels)
    {
        Level->RouteActorInitialize();
    }
}

ULevel* UWorld::GetCurrentLevel() const
{
	return CurrentLevel;
}

ULevel* UWorld::GetPersistentLevel() const
{
	return PersistentLevel;
}

void UWorld::LoadLevel(ULevel* Level)
{
	CurrentLevel = Level;
	Initialize();
	BeginPlay();
}

UWorld* UWorld::CreateWorld(EWorldType InWorldType)
{
	UWorld* NewWorld = NewObject<UWorld>(GetTransientPackage());
	NewWorld->WorldType = InWorldType;
	NewWorld->Initialize();

	return NewWorld;
}

UWorld* UWorld::CreateWorldWithEmptyLevel(EWorldType InWorldType)
{
	UWorld* NewWorld = CreateWorld(InWorldType);

	// 레벨 생성 및 등록
	ULevel* Level = NewObject<ULevel>(NewWorld);
	NewWorld->PersistentLevel = Level;
	NewWorld->CurrentLevel = Level;

	// 레벨 초기화
	Level->OwningWorld = NewWorld;
	Level->Initialize();

	return NewWorld;
}

UWorld* UWorld::CreateWorldWithLevel(const FArchive& Archive, EWorldType InWorldType)
{
	UWorld* NewWorld = CreateWorldWithEmptyLevel(InWorldType);

	// 레벨 불러오기
	FArchive LevelArchive = Archive.GetArchive("Level");
	NewWorld->PersistentLevel->Deserialize(LevelArchive);
	
	return NewWorld;
}

AActor* UWorld::SpawnActor(UClass* Class, FTransform const* Transform)
{
	if (!CurrentLevel)
	{
		return nullptr;
	}

	// 새로운 액터 생성
	AActor* Actor = NewObject<AActor>(CurrentLevel, Class);

	if (Transform)
	{
		Actor->SetTransform(*Transform);
	}

	// 생명주기 실행
	Actor->Initialize();
	Actor->PostSpawnInitialize();

	// 현재 레벨에 추가
	CurrentLevel->Actors.push_back(Actor);

	return Actor;
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
			Actors[i] = nullptr;
			return;
		}
	}
}

UWorld* UWorld::GetWorld() const
{
	// It's me!

	// 언리얼 소스코드가 이래 되어있음
	// 주석에서는 const 한정자만 지우고 반환하는 용도로는 괜찮다고 설명함...
	return const_cast<UWorld*>(this);
}

bool UWorld::DestroyActor(AActor* Actor)
{
	if (Actor == nullptr)
	{
		return false;
	}

	RemoveActor(Actor, false);

	Actor->UnregisterAllComponents();
	Actor->UninitializeComponents();

	DestroyObject(Actor);

	return true;
}
