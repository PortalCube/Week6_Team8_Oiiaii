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
	bool bTickInEditor = false;
	bool bHasBegunPlay = false;
	bool bRegistered = false;
	bool bIsEditorOnlyActor = false;

	virtual void Serialize(FArchive& Archive) override;
	


public:

	////////////////////////////////////////////////////////////
	// Lifecycle
	////////////////////////////////////////////////////////////

	virtual void Initialize() override;
	virtual void Release() override;

	virtual void Register();
	virtual void Unregister();

	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);
	virtual void EndPlay();



	////////////////////////////////////////////////////////////
	// Getter, Setter
	////////////////////////////////////////////////////////////

	virtual class ULevel* GetLevel() const;
	virtual class UWorld* GetWorld() const;

	virtual bool IsEditorOnly() const override;



	////////////////////////////////////////////////////////////
	// Component
	////////////////////////////////////////////////////////////

	void SetRootComponent(USceneComponent* Component);
	USceneComponent* GetRootComponent() const { return RootComponent; }

	UActorComponent* AddComponent(UClass* Class);
	void RemoveComponent(UActorComponent* Component);

	const TArray<UActorComponent*>& GetOwnedComponents() const { return OwnedComponents; }

	// 액터의 자식 컴포넌트 생성 및 등록
	template <UObjectType T>
	T* CreateDefaultSubobject();

	template <UObjectType T>
	void CreateEditorOnlyDefaultSubobject();



	////////////////////////////////////////////////////////////
	// Transform
	////////////////////////////////////////////////////////////

	FTransform GetTransform() const;
	void SetTransform(const FTransform& NewTransform);

	// 컴포넌트에 Delete 세팅
	void MarkComponentsTransformDirty();



	////////////////////////////////////////////////////////////
	// Check Lifecycle
	////////////////////////////////////////////////////////////

	bool HasBegunPlay() const { return bHasBegunPlay; }
	bool GetTickEnabled() const;



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
