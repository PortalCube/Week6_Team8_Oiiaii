#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Engine/Types/EngineTypes.h"
#include "Runtime/Engine/ULevel.h"

// UWorld
// UnrealEngine/Engine/Source/Runtime/Engine/Classes/Engine/World.h:931

class AActor;
class FTransform;

class UWorld final : public UObject
{
	DECLARE_UCLASS(UWorld, UObject)
	GENERATED_BODY()

private:
		
	EWorldType WorldType;

	// TODO: Sub Level
	// TArray<ULevel*> Level;

	ULevel* CurrentLevel;

public:

	////////////////////////////////////////////////////////////
	// World 생명주기
	////////////////////////////////////////////////////////////

	void Initialize() override;
	void BeginPlay();
	void Tick(float DeltaSeconds);
	void EndPlay();
	void CleanupWorld();
	void Release() override;



	////////////////////////////////////////////////////////////
	// Level
	////////////////////////////////////////////////////////////

	ULevel* GetCurrentLevel() const;
	void LoadLevel(ULevel* Level);



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

	// TODO: 구현 예정

	// 주어진 Ray에 대해 충돌하는 물체가 있는지 확인합니다.
	// bool LineTraceTest(struct FHitResult& OutHit, const FVector& Start, const FVector& End) const;

	// 주어진 Ray에 대해 충돌하는 가장 가까운 물체를 찾습니다.
	// bool LineTraceSingle(struct FHitResult& OutHit, const FVector& Start, const FVector& End) const;

	// 주어진 Ray에 대해 충돌하는 모든 물체를 찾습니다.
	// bool LineTraceMulti(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End) const;

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
	if (!CurrentLevel)
	{
		return nullptr;
	}

	// 새로운 액터 생성
	T* Actor = NewObject<T>(this, Class);

	if (Transform)
	{
		Actor->SetTransform(*Transform);
	}

	// 생명주기 실행
	Actor->Initialize();
	Actor->Register(*CurrentLevel);

	// 현재 레벨에 추가
	CurrentLevel->Actors.push_back(Actor);

	return Actor;

	return nullptr;
}
