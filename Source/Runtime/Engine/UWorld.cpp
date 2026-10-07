#include "Runtime/Serialization/FJsonDataReader.h"
#include "UWorld.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/UEngine.h"
#include "Runtime/Serialization/FJson.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Serialization/FJsonDataWriter.h"

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

	for (auto Level : Levels)
	{
		if (Level)
		{
			Level->Register();
		}
	}
	
	BeginPlay();
}

void UWorld::BeginPlay()
{
	if (bBegunPlay || (WorldType != EWorldType::Game && WorldType != EWorldType::PIE))
	{
		return;
	}

	bBegunPlay = true;

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
	bBegunPlay = false;

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
	EndPlay();
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
			Level->Register();
		}
	}
}

void UWorld::ClearWorldComponents()
{
	for (ULevel* Level : Levels)
	{
		Level->Unregister();
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

UWorld* UWorld::CreateWorldWithLevel(const FJson& Snapshot, EWorldType InWorldType)
{
	UWorld* NewWorld = CreateWorldWithEmptyLevel(InWorldType);
	FJsonDataReader Reader(Snapshot.GetJson("Level").GetJSON());
	Reader.Serialize(NewWorld->PersistentLevel);
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

	Actor->Initialize();

	// 현재 레벨에 추가
	CurrentLevel->Actors.push_back(Actor);

	Actor->Register();

	if (Transform)
	{
		Actor->SetTransform(*Transform);
	}

	if (bBegunPlay)
	{
		Actor->BeginPlay();
	}

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

UWorld* UWorld::DuplicateWorld(EWorldType InWorldType)
{
	FJsonDataWriter Writer;
	Writer.Serialize(PersistentLevel);

	auto Data = Writer.CloneJSON();
	for (auto& Record : Data)
	{
		Record.erase("UUID");
	}

	UWorld* Copy = CreateWorldWithEmptyLevel(InWorldType);

	FJsonDataReader Reader(Data);
	Reader.Serialize(Copy->PersistentLevel);

	return Copy;
}

EWorldType UWorld::GetWorldType() const
{
	return WorldType;
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

	// 배열에서 제거
	RemoveActor(Actor, false);

	// 액터의 종료
	Actor->EndPlay();
	Actor->Unregister();
	DestroyObject(Actor);

	return true;
}
