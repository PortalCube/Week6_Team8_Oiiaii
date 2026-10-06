#pragma once

#include "Runtime/Geometry/FTransform.h"
#include "Runtime/Components/UActorComponent.h"
#include "ThirdParty/Json/json.hpp"
#include "Runtime/CoreUObject/UObject.h"

class ULevel;
class AActor;
class FArchive;

class USceneComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(USceneComponent, UActorComponent)
	friend class AActor;

public:

	////////////////////////////////////////////////////////////
	// Get Owner
	////////////////////////////////////////////////////////////

	// 생성자 (Initalize) 함수에서 호출
	void SetupAttachment(USceneComponent* InParent);

	// bHasBegunPlay 이후에 컴포넌트를 붙일 때 호출
	bool AttachToComponent(USceneComponent* InParent);

	// 현재 컴포넌트의 부모 컴포넌트를 분리
	void DetachFromComponent();

	void SetAttachParent(USceneComponent* NewAttachParent);



	////////////////////////////////////////////////////////////
	// 직렬화, 역직렬화
	////////////////////////////////////////////////////////////

	virtual void Serialize(FArchive& Archive) override;


protected:
	FTransform RelativeTransform;

	// 파생 클래스에서 Transfrom 변경에 따라 반응.
	virtual void OnTransformChanged() {}

public:

	const FMatrix& GetGlobalTransformMatrix() const { return GetGlobalTransform().GetMatrix(); }

	// 월드 행렬의 역행렬. 스케일이 0에 가까워 역행렬이 없으면 nullptr.
	const FMatrix* GetGlobalInverseMatrix() const;

	// Transform이 바뀔 때 알림. 액터 전체 컴포넌트에 전파
	void MarkActorTransformDirty();

	
	////////////////////////////////////////////////////////////
	// Transform
	////////////////////////////////////////////////////////////

	const FTransform& GetRelativeTransform() const { return RelativeTransform; }
	virtual void SetRelativeTransform(const FTransform& RelativeTransform);
	const FTransform& GetGlobalTransform() const;

	virtual const FVector& GetRelativeLocation() const;
	virtual void SetRelativeLocation(const FVector& RelativeLocation);

	virtual const FQuaternion& GetRelativeRotation() const;
	virtual void SetRelativeRotation(const FVector& RelativeRotation);
	virtual void SetRelativeRotation(const FQuaternion& RelativeRotation);

	virtual const FVector& GetRelativeScale() const;
	virtual void SetRelativeScale(const FVector& RelativeScale);

	void SetBatchIndex(int32 Index) { BatchIndex = Index; }
	int32 GetBatchIndex() const { return BatchIndex; }

	void SetInheritRotation(bool bInherit)
	{
		bInheritRotation = bInherit;
		bGlobalDirty = true;
	}

protected:
	bool bInheritRotation = true;
	int32 BatchIndex = -1;

private:
	USceneComponent* GetTransformParent() const;

	// 현재 컴포넌트가 부착된 부모 USceneComponent
	USceneComponent* AttachParent = nullptr;

	// 월드 Transform 캐시. 부모의 GlobalVersion이 바뀌면 자식도 자동으로 재계산된다.
	mutable FTransform CachedGlobal;
	mutable FMatrix CachedGlobalInverse;
	mutable uint32 CachedInverseVersion = UINT32_MAX;
	mutable bool bCachedInverseValid = false; // 역행렬 계산 실패도 캐시한다
	mutable const USceneComponent* CachedParent = nullptr;
	mutable uint32 CachedParentVersion = 0;
	mutable uint32 GlobalVersion = 0;
	mutable bool bGlobalDirty = true;
};
