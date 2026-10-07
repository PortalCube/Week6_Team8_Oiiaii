#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Engine/Types/EngineTypes.h"
#include "Runtime/Engine/ULevel.h"

// UWorld
// UnrealEngine/Engine/Source/Runtime/Engine/Classes/Engine/World.h:931

class FJson;
class AActor;
class FTransform;

class UWorld final : public UObject
{
	DECLARE_UCLASS(UWorld, UObject)
	GENERATED_BODY()

private:
		
	EWorldType WorldType;

	// 월드가 가진 모든 레벨 목록.
	// 지금은 생성 직후를 제외하고는 요소 갯수가 항상 1개로 고정되어야 함
	TArray<ULevel*> Levels;

	// 월드와 생명을 같이하는 멤버 변수.
	// PersistentLevel과 World는 같이 생성되고 같이 삭제된다.
	ULevel* PersistentLevel;

	// 에디터에서 현재 선택된 레벨.
	ULevel* CurrentLevel;

public:

	bool bBegunPlay = false;

	////////////////////////////////////////////////////////////
	// World 생명주기
	////////////////////////////////////////////////////////////

	void Initialize() override;
	void Release() override;

	void InitWorld();
	void CleanupWorld();

	void BeginPlay();
	void Tick(float DeltaSeconds);
	void EndPlay();

	void UpdateWorldComponents();
	void ClearWorldComponents();

	void InitializeActorsForPlay();



	////////////////////////////////////////////////////////////
	// Level
	////////////////////////////////////////////////////////////

	ULevel* GetCurrentLevel() const;
	ULevel* GetPersistentLevel() const;
	void LoadLevel(ULevel* Level);

	// 나중에 SubLevel을 구현하려면 이 함수 구현
	// void AddToWorld(ULevel* Level, const FTransform& LevelTransform);

	static UWorld* CreateWorld(EWorldType InWorldType);
	static UWorld* CreateWorldWithEmptyLevel(EWorldType InWorldType);
	static UWorld* CreateWorldWithLevel(const FJson& Snapshot, EWorldType InWorldType);



	////////////////////////////////////////////////////////////
	// Actor
	////////////////////////////////////////////////////////////

	template <AActorType T>
	T* SpawnActor(UClass* Class, FTransform const* Transform = nullptr);

	AActor* SpawnActor(UClass* Class, FTransform const* Transform = nullptr);

	bool DestroyActor(AActor* Actor);

	void RemoveActor(AActor* Actor, bool bShouldModifyLevel = false);



	////////////////////////////////////////////////////////////
	// Raycast & Physics
	////////////////////////////////////////////////////////////

	// TODO: 나중에 필요하면 하나씩 구현

	// 주어진 Ray에 대해 충돌하는 물체가 있는지 확인합니다.
	// bool LineTraceTest(struct FHitResult& OutHit, const FVector& Start, const FVector& End) const;

	// 주어진 Ray에 대해 충돌하는 가장 가까운 물체를 찾습니다.
	// bool LineTraceSingle(struct FHitResult& OutHit, const FVector& Start, const FVector& End) const;

	// 주어진 Ray에 대해 충돌하는 모든 물체를 찾습니다.
	// bool LineTraceMulti(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End) const;



	////////////////////////////////////////////////////////////
	// 직렬화
	////////////////////////////////////////////////////////////

	UWorld* DuplicateWorld(EWorldType InWorldType);

	EWorldType GetWorldType() const;


	////////////////////////////////////////////////////////////
	// Time
	////////////////////////////////////////////////////////////

	double TimeSeconds;
	double DeltaTimeSeconds;



	////////////////////////////////////////////////////////////
	// GetWorld
	////////////////////////////////////////////////////////////

	virtual class UWorld* GetWorld() const override;

};



////////////////////////////////////////////////////////////
// 템플릿 함수 구현부
////////////////////////////////////////////////////////////

template <AActorType T>
T* UWorld::SpawnActor(UClass* Class, FTransform const* Transform)
{
	return SpawnActor(Class, Transform)->Cast<T>();
}
