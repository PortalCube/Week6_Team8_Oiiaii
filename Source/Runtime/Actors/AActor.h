#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Components/USceneComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include <type_traits>
#include <concepts>

class ULevel;

class AActor : public UObject
{
	DECLARE_UCLASS(AActor, UObject)
	GENERATED_BODY()

	friend class ULevel;

protected:

	// 액터가 소유하는 컴포넌트들
	TArray<class UActorComponent*> OwnedComponents;
	USceneComponent* RootComponent = nullptr;
	
	bool bTickEnabled = false;
	bool bHasBegunPlay = false;

	bool bIsEditorOnlyActor = false;

	virtual void Serialize(FArchive& Archive) override;


public:

	////////////////////////////////////////////////////////////
	// Lifecycle
	////////////////////////////////////////////////////////////

	virtual void Initialize() override;
	virtual void Release() override;

	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);
	virtual void EndPlay();

	virtual void RegisterAllComponents();
	virtual void UnregisterAllComponents();

	// 액터가 소유하는 컴포넌트를 초기화하기 전 단계.
	// 액터의 데이터를 먼저 초기화함
	virtual void PreInitializeComponents() {}

	// 컴포넌트 사이에서 커플링이 필요하다면 여기서 실행
	virtual void PostInitializeComponents() {}

	virtual void InitializeComponents();
	virtual void UninitializeComponents();

	// SpawnActor
	// 새로운 액터가 생성된 이후에 호출됩니다.
	virtual void PostSpawnInitialize();

	// 액터가 월드에 스폰된 이후에 호출됩니다.
	// AActor 오버라이딩 전용
	virtual void PostActorCreated() {};




	////////////////////////////////////////////////////////////
	// Getter, Setter
	////////////////////////////////////////////////////////////

	virtual class ULevel* GetLevel() const;
	virtual class UWorld* GetWorld() const;

	virtual bool IsEditorOnly() const override;
	virtual bool IsSelectable() const { return true; }



	////////////////////////////////////////////////////////////
	// Component
	////////////////////////////////////////////////////////////





	void SetRootComponent(USceneComponent* Component);
	USceneComponent* GetRootComponent() const { return RootComponent; }

	const TArray<UActorComponent*>& GetOwnedComponents() const { return OwnedComponents; }

	// 액터의 자식 컴포넌트 생성 및 등록
	template <UObjectType T>
	T* CreateDefaultSubobject();

	template <UObjectType T>
	void CreateEditorOnlyDefaultSubobject();



	////////////////////////////////////////////////////////////
	// Transform
	////////////////////////////////////////////////////////////

	FTransform GetTransform() const { return RootComponent ? RootComponent->GetRelativeTransform() : FTransform{}; }
	void SetTransform(const FTransform& NewTransform)
	{
		if (RootComponent)
			RootComponent->SetRelativeTransform(NewTransform);
	}

	// 컴포넌트에 Delete 세팅
	void MarkComponentsTransformDirty();



	////////////////////////////////////////////////////////////
	// Check Lifecycle
	////////////////////////////////////////////////////////////

	bool HasBegunPlay() const { return bHasBegunPlay; }
	bool GetTickEnabled() const { return bTickEnabled; }



	////////////////////////////////////////////////////////////
	// Destroy
	////////////////////////////////////////////////////////////

	void Destroy();
};



////////////////////////////////////////////////////////////
// 템플릿 함수 구현부
////////////////////////////////////////////////////////////

template <UObjectType T>
inline T* AActor::CreateDefaultSubobject()
{
	T* Subobject = Super::CreateDefaultSubobject<T>();

	// 현재 컴포넌트 목록에 등록시키기
	OwnedComponents.push_back(Subobject);

	return Subobject;
}
