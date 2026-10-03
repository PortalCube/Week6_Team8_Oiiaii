#pragma once

#include "Editor/EditorViewport/FEditorViewportClient.h"
#include "Editor/Gizmo/FGizmo.h"
#include "Editor/Core/FEditorState.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/TWeakObjectPtr.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Components/UTextComponent.h"
#include "Runtime/UI/SSplitter.h"
#include "Editor/EditorViewport/FEditorViewport.h"
#include "Editor/EditorViewport/FEditorViewportLayout.h"

enum class EEditorPrimitiveType : uint8
{
	Cube,
	Cylinder,
	Sphere,
	Billboard,
	Spotlight,
};

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
	bool bShowBenchmark = true;
	double LastPickingMs = 0.0;
	double AccumulatedPickingMs = 0.0;
	int32 PickingAttempts = 0;

public:
	FEditor()
	{
		//ViewportLayout.Initialize(Viewports);
	}
	void Initialize(USceneManager* SceneManager);
	void Shutdown();

	void Process();

	void NewScene();
	void SaveScene(const FString& Path);
	void LoadScene(const FString& Path);
	bool CheckSceneExists();

	void AddViewport(FEditorViewportClient Viewport);
	void InitViewports();
	void ResizeView(FEditorState::SplitViewMode mode);
	void DeleteViewport(int32 IndexOfViewport);
	SViewport* GetActiveViewport();

	void UpdateCamera();

	bool SelectActor(AActor* Actor);
	void UnSelectActor();
	AActor* GetSelectedActor() const { return SelectedActor.Get(); }
	[[nodiscard]] bool ActorSelected() const { return SelectedActor.IsValid(); }
	[[nodiscard]] bool ObjectSelected() const { return SelectedActor.IsValid(); }

	// Viewport관련
	FEditorViewport (&GetViewports())[MAX_VIEWPORT_COUNT] { return Viewports; }

	FEditorViewportLayout& GetViewportLayout() { return ViewportLayout; }

	[[nodiscard]] UScene* GetCurrentScene() const
	{
		return SceneManager ? SceneManager->CurrentScene : nullptr;
	}
	void SpawnActorToCurrentScene(UClass* Type, int Count = 1);
	// 피킹 등에서 현재 씬의 렌더링 대상 컴포넌트가 필요할 때 사용
	[[nodiscard]] const TArray<UPrimitiveComponent*>& GetPrimitiveComponents() const;
	FGizmo& GetGizmo() { return Gizmo; }
	FRenderResourceLibrary* GetRendererLibrary();

	void ClearSelectionForGC();

	void SaveState();
	void LoadState();
	void SetViewLayout(FEditorState::SplitViewMode mode);
	UTextComponent* GetTextcomp() { return SelectedActorTextComp; }

	void ResetPickingStats()
	{
		LastPickingMs = 0.0;
		AccumulatedPickingMs = 0.0;
		PickingAttempts = 0;
	}

private:
	USceneManager* SceneManager =
	    nullptr; // 씬을 다중으로 가질 수 있도록 구조개선 가능-이경우 에디터쪽에
	             // 클래스를 추가해 씬과 FEditorViewportClient들을 연관
	FGizmo Gizmo;
	TWeakObjectPtr<AActor> SelectedActor;
	TWeakObjectPtr<UTextComponent> SelectedActorTextComp;

	// Viewport관련
	FEditorViewport Viewports[MAX_VIEWPORT_COUNT];
	FEditorViewportLayout ViewportLayout;

};
