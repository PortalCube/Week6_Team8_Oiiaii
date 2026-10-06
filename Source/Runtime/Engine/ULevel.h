#pragma once

#include "Runtime/Actors/AActor.h"
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UPrimitiveComponent.h"
#include "Runtime/Components/USceneComponent.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Engine/FSceneBVH.h"
#include "Runtime/Engine/FCulling.h"
#include <concepts>
#include <type_traits>

#include "ThirdParty/Json/json.hpp"

class ULevel final : public UObject
{
	DECLARE_UCLASS(ULevel, UObject)
	GENERATED_BODY()

	friend class UWorld;

public:
	////////////////////////////////////////////////////////////
	// Lifecycle
	////////////////////////////////////////////////////////////

	void Initialize() override;
	void Release() override;

	void UpdateLevelComponents();
	void ClearLevelComponents();

	void BeginPlay();
	void Tick(float DeltaTime);
	void EndPlay();

	void RouteActorInitialize();
	void CleanupLevel();

	bool HasBegunPlay() const { return bHasBegunPlay; }



	////////////////////////////////////////////////////////////
	// Actor, Component List
	////////////////////////////////////////////////////////////

	// 렌더링 컴포넌트 목록 반환
	const TArray<UPrimitiveComponent*>& GetRenderComponents() const;

	// 액터 목록 반환
	const TArray<AActor*>& GetActors() const { return Actors; }

	// 컬링 전용 월드 AABB 배열 (RenderComponents와 같은 인덱스)
	const TArray<FAxisAlignedBoundingBox>& GetCullDataList() const { return CullDataList; }

	// 캐시해두는 오클루전 대상 Getter
	const TArray<uint8>& GetOcclusionTargetFlags() const { return OcclusionTargetFlags; }

	void AddRenderComponent(UPrimitiveComponent* prim);
	void RemoveRenderComponent(UPrimitiveComponent* prim);



	////////////////////////////////////////////////////////////
	// 직렬화, 역직렬화
	////////////////////////////////////////////////////////////

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;



	////////////////////////////////////////////////////////////
	// BVH
	////////////////////////////////////////////////////////////

	FSceneBVH& GetSceneBVH() { return SceneBVH; }
	const FSceneBVH& GetSceneBVH() const { return SceneBVH; }



	////////////////////////////////////////////////////////////
	// Dirty Flag
	////////////////////////////////////////////////////////////

	void MarkBoundsDirty(UPrimitiveComponent* Prim);

	// dirty 컴포넌트만 월드 AABB 재계산. 렌더 전에 프레임당 1회
	void UpdateDirtyBounds();

	UWorld* OwningWorld;

private:

	TMap<UPrimitiveComponent*, size_t> RenderIndices;

	FRenderResourceLibrary* RenderResourceLibrary = nullptr;
	bool bInitialized = false;
	bool bHasBegunPlay = false;

	FSceneBVH SceneBVH;

	// 액터 목록 (Update용)
	TArray<AActor*> Actors;

	// 렌더링큐 (Draw용)
	TArray<UPrimitiveComponent*> RenderComponents;

	// RenderComponents와 같은 인덱스
	TArray<FAxisAlignedBoundingBox> CullDataList;

	// 이번 프레임 재계산 대상
	TArray<UPrimitiveComponent*> DirtyBoundsList;

	// 캐시해둘 오클루전 대상
	TArray<uint8> OcclusionTargetFlags;
};
