#include "ULevel.h"

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Components/UPrimitiveComponent.h"
#include "Runtime/Components/USceneComponent.h"
#include "Runtime/Serialization/FArchive.h"
#include "Runtime/Engine/UWorld.h"

#include <algorithm>

#include "Runtime/CoreUObject/TObjectIterator.h"
#include "Runtime/Core/Log.h"

IMPLEMENT_UCLASS(ULevel, UObject)
UCLASS_META(ULevel, SerializeName, "Level")

const TArray<UPrimitiveComponent*>& ULevel::GetRenderComponents() const
{
	return RenderComponents;
}

void ULevel::Initialize()
{
	if (bInitialized)
	{
		return;
	}

	Super::Initialize();
	bInitialized = true;
}

void ULevel::Release()
{
	for (auto Actor : Actors)
	{
		if (Actor)
		{
			DestroyObject(Actor);
		}
	}

	RenderComponents.clear();
	CullDataList.clear();
	DirtyBoundsList.clear();
	OcclusionTargetFlags.clear();
	RenderResourceLibrary = nullptr;
	bInitialized = false;

	Super::Release();
}

void ULevel::BeginPlay()
{
	if (bHasBegunPlay)
	{
		return;
	}

	bHasBegunPlay = true;

	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			Actor->BeginPlay();
		}
	}
}

void ULevel::Tick(float DeltaTime)
{
	if (!bHasBegunPlay)
	{
		return;
	}

	for (AActor* Actor : Actors)
	{
		if (Actor && Actor->GetTickEnabled())
		{
			Actor->Tick(DeltaTime);
		}
	}
}

void ULevel::EndPlay()
{
	if (!bHasBegunPlay)
	{
		return;
	}

	bHasBegunPlay = false;

	for (auto Actor : Actors)
	{
		if (Actor)
		{
			Actor->EndPlay();
		}
	}
}

void ULevel::UpdateLevelComponents()
{
	for (auto Actor : Actors)
	{
		if (Actor)
		{
			Actor->RegisterAllComponents();
		}
	}
}

void ULevel::ClearLevelComponents()
{
	for (auto Actor : Actors)
	{
		if (Actor)
		{
			Actor->UnregisterAllComponents();
		}
	}
}

void ULevel::RouteActorInitialize()
{
	for (auto Actor : Actors)
	{
		if (Actor)
		{
			Actor->InitializeComponents();
		}
	}
}

void ULevel::CleanupLevel()
{
	for (int i = 0; i < Actors.size(); ++i)
	{
		if (Actors[i])
		{
			Actors[i]->UninitializeComponents();

			DestroyObject(Actors[i]);

			Actors[i] = nullptr;
		}
	}
}

void ULevel::Serialize(FArchive& Archive)
{
	Super::Serialize(Archive);

	TArray<AActor*> SavedActors;

	if (Archive.IsWriting())
	{
		for (AActor* Actor : Actors)
		{
			if (Actor && !Actor->IsEditorOnly())
			{
				SavedActors.push_back(Actor);
			}
		}
	}

	int32 Count = Archive.BeginArray("Actors");
	{
		if (Archive.IsReading())
		{
			Actors.resize(Count, nullptr);
		}
		else
		{
			Count = static_cast<int32>(SavedActors.size());
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			AActor*& Actor = Archive.IsReading() ? Actors[Index] : SavedActors[Index];
			Archive.Reference("", Actor);
		}
	}
	Archive.EndArray();
}

void ULevel::AddRenderComponent(UPrimitiveComponent* prim)
{
	if (prim == nullptr)
		return;

	if (std::find(RenderComponents.begin(), RenderComponents.end(), prim) ==
	    RenderComponents.end())
	{
		// RenderComponents에 넣기 전에 인덱스 설정
		const int32 NewIndex = static_cast<int32>(RenderComponents.size());
		prim->SetSceneIndex(NewIndex);
		prim->SetBatchIndex(NewIndex);

		RenderComponents.push_back(prim);
		SceneBVH.AddObject(prim);

		// 처음엔 일단 그리자
		CullDataList.push_back(MakeAlwaysVisibleCullData());
		// 오클루전 대상에도 추가
		OcclusionTargetFlags.push_back(0);
		MarkBoundsDirty(prim);
	}
}

void ULevel::RemoveRenderComponent(UPrimitiveComponent* prim)
{
	if (prim == nullptr || prim->GetSceneIndex() < 0)
		return;

	// TODO 제거할 때 마지막 요소와 교환하는 방식의 Swap and Pop으로 처리하도록 수정할 것
	std::erase(RenderComponents, prim);
	SceneBVH.RemoveObject(prim);

	const size_t Index = static_cast<size_t>(prim->GetSceneIndex());
	CullDataList.erase(CullDataList.begin() + Index);

	// 오클루전 대상에서 제거
	OcclusionTargetFlags.erase(OcclusionTargetFlags.begin() + Index);

	// 당겨진 원소들의 인덱스 멤버 갱신
	for (size_t i = Index; i < RenderComponents.size(); ++i)
	{
		RenderComponents[i]->SetSceneIndex(static_cast<int32>(i));
		RenderComponents[i]->SetBatchIndex(static_cast<int32>(i));
	}

	// 파괴될 포인터가 dirty 목록에 남지 않게
	if (prim->GetBoundDirtyQueued())
	{
		std::erase(DirtyBoundsList, prim);
		prim->SetBoundDirtyQueued(false);
	}

	prim->SetSceneIndex(-1);
}

void ULevel::MarkBoundsDirty(UPrimitiveComponent* Prim)
{
	// 씬에 아직 추가 전이거나 이미 대기 중이면 무시
	if (Prim == nullptr || Prim->GetSceneIndex() < 0 || Prim->GetBoundDirtyQueued())
		return;

	Prim->SetBoundDirtyQueued(true);
	DirtyBoundsList.push_back(Prim);
}

void ULevel::UpdateDirtyBounds()
{
	for (UPrimitiveComponent* Prim : DirtyBoundsList)
	{
		Prim->SetBoundDirtyQueued(false);
		Prim->UpdateWorldBounds();
		int32 Index = static_cast<size_t>(Prim->GetSceneIndex());
		CullDataList[Index] = Prim->GetWorldBounds();
		OcclusionTargetFlags[Index] = Prim->IsOcclusionTarget() ? 1 : 0;
	}
	DirtyBoundsList.clear();
}
