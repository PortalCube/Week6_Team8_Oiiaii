#pragma once

#include "Editor/EditorViewport/FEditorViewportClient.h"
#include "Editor/Gizmo/FGizmo.h"
#include "Editor/Core/FEditorState.h"
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/TWeakObjectPtr.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/ASelectedTextActor.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Components/UTextComponent.h"

#include "Runtime/Engine/UWorld.h"
#include "Runtime/Engine/ULevel.h"

#include "Runtime/UI/SSplitter.h"
#include "Editor/EditorViewport/SEditorViewport.h"
#include "Editor/EditorViewport/FEditorViewportLayout.h"

#include <memory>

enum class EEditorPrimitiveType : uint8
{
	Cube,
	Cylinder,
	Sphere,
	Billboard,
	Spotlight,
};

class UEditorEngine;
class PIEManager;
struct FRequestPlaySessionParams;

class FEditor
{
public:
	FTransform SelectedTransform;
	FVector SelectedEulerDegDisplay;

	// TODO: 이건 Scene에 들어가야함. 아마 아래와 같은 컴포넌트가 부착된 액터로 들어가야할 것
	// https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UDirectionalLightComponent
	FLightConstants GlobalLight;

	FEditorState State;

	// 피킹 경로 선택 및 측정. 검증이 끝나면 제거한다.
	bool bUseBVHPicking = true;
	bool bHideUI = false;
	// F11. bHideUI가 숨기는 창에 더해 툴바까지 숨긴다.
	bool bZenMode = false;
	bool bShowBenchmark = false;
	double LastPickingMs = 0.0;
	double AccumulatedPickingMs = 0.0;
	int32 PickingAttempts = 0;

public:
	void Initialize(UEditorEngine* EditorEngine);
	void Shutdown();

	void Process();

	void OnWorldLoaded(FWorldContext& Context);

	// Scene
	void NewScene();
	void SaveScene(const FString& Path);
	void LoadScene(const FString& Path);

	// Viewport
	SEditorViewport* GetActiveViewport();
	void SetViewLayout(FEditorState::SplitViewMode mode);
	FEditorViewportLayout& GetViewportLayout() { return ViewportLayout; }
	FEditorRenderContext GetEditorRenderContext(SEditorViewport& EditorViewport, FVisualizerRegistry* VisualizerRegistry);
	SEditorViewport* GetPerspectiveViewport();

	// Actor
	bool SelectActor(AActor* Actor);
	void UnSelectActor();
	AActor* GetSelectedActor() const { return SelectedActor.Get(); }
	bool ActorSelected() const { return SelectedActor.IsValid(); }
	bool ObjectSelected() const { return SelectedActor.IsValid(); }
	UWorld* GetCurrentWorld() const;
	ULevel* GetCurrentLevel() const;
	void SpawnActorToCurrentScene(UClass* Type, int Count = 1);

	UTextComponent* GetTextcomp() { return SelectedActorTextComp; }
	
	// 피킹 등에서 현재 씬의 렌더링 대상 컴포넌트가 필요할 때 사용
	const TArray<UPrimitiveComponent*>& GetPrimitiveComponents() const;
	FGizmo& GetGizmo() { return Gizmo; }

	void ClearSelectionForGC();

	// State
	void SaveState();
	void LoadState();
	void ResetPickingStats()
	{
		LastPickingMs = 0.0;
		AccumulatedPickingMs = 0.0;
		PickingAttempts = 0;
	}

	// PIE
	bool RequestStartPIE(const FRequestPlaySessionParams& Params);
	void RequestEndPIE();
	void ProcessPIERequests();

private:
	UEditorEngine* EditorEngine = nullptr;
	FGizmo Gizmo;
	TWeakObjectPtr<AActor> SelectedActor;
	TWeakObjectPtr<ASelectedTextActor> SelectedActorTextActor;
	TWeakObjectPtr<UTextComponent> SelectedActorTextComp;

	// Viewport
	FEditorViewportLayout ViewportLayout;
	std::unique_ptr<PIEManager> PlayManager;
};
