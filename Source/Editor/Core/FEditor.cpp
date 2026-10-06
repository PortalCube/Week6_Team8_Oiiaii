#include "FEditor.h"

#include <numbers>

#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/ABillboardActor.h"
#include "Runtime/Actors/ACubeActor.h"
#include "Runtime/Actors/ACylinderActor.h"
#include "Runtime/Actors/ASphereActor.h"
#include "Runtime/Actors/ASpotlightActor.h"
#include "Runtime/Actors/ASelectedTextActor.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Components/USceneComponent.h"
#include "Runtime/Engine/FWorldContext.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Engine/FSceneBVH.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Math/Random.h"
#include "Runtime/Core/Globals.h"
#include "Editor/Engine/UEditorEngine.h"
#include "Editor/PlayInEditor/FPlayInEditorManager.h"


void FEditor::Initialize(UEditorEngine* EditorEngine)
{
	State.ReadFromFile();
	Gizmo.Initialize();

	UWorld* World = EditorEngine->GetEditorWorld();

	PlayManager = std::make_unique<PIEManager>(*EditorEngine);

	this->EditorEngine = EditorEngine;
}

void FEditor::Shutdown()
{
	SaveState();
	State.FlushToFile();
}

void FEditor::Process()
{
	// F11로 젠 모드(UI 숨김 모드) 진입
	if (FInputManager::Get().IsKeyDown(VK_F11))
	{
		bZenMode = !bZenMode;
	}

	if (SelectedActor)
	{
		// 선택된 액터 Delete 키로 삭제
		if (FInputManager::Get().IsKeyPressed(VK_DELETE))
		{
			AActor* Target = SelectedActor;
			UnSelectActor();
			Target->Destroy();
		}

		// BVH 갱신
		{
			USceneComponent* Root = SelectedActor->GetRootComponent();
			const bool bChanged = Root && !(Root->GetRelativeTransform() == SelectedTransform);

			SelectedActor->SetTransform(SelectedTransform);

			// Transform이 변경되었을 때만 Refit
			if (bChanged)
			{
				RefitActorInBVH(GetCurrentLevel()->GetSceneBVH(), SelectedActor);
			}
		}
	}

	// 현재 상태를 State에 저장
	SaveState();

	// State를 파일에 주기적으로 자동 저장
	State.Tick(FTimeManager::GetDeltaTime());
}

void FEditor::OnWorldLoaded(FWorldContext& Context)
{
	if (Context.WorldType == EWorldType::Editor)
	{
		SelectedActorTextActor = GetCurrentWorld()->SpawnActor<ASelectedTextActor>(ASelectedTextActor::StaticClass());

		SelectedActorTextComp = SelectedActorTextActor->TextComponent;

		for (auto& Viewport : ViewportLayout.Viewports)
		{
			Viewport.GetClient().SetWorldContext(EditorEngine->GetEditorWorldContext());
		}
	}
	else if (Context.WorldType == EWorldType::PIE)
	{
		ViewportLayout.ActiveViewport->GetClient().SetWorldContext(EditorEngine->GetPIEWorldContext());
	}
	
}

void FEditor::SaveState()
{
	SEditorViewport* Viewport = GetPerspectiveViewport();
	if (!Viewport)
	{
		return;
	}

	const FCamera& Camera = Viewport->GetClient().GetViewportCamera();
	State.SetCameraLocation(Camera.GetPosition());
	State.SetCameraPitch(Camera.GetPitch());
	State.SetCameraYaw(Camera.GetYaw());
	State.SetCameraFOV(Camera.GetProjection().GetFOV());
	State.SetGridCellSize(Viewport->GetClient().GetGrid().GetCellSize());
	State.SetGizmoMode(static_cast<uint8>(Gizmo.Mode));
	State.SetGizmoSpace(static_cast<uint8>(Gizmo.GetSpace()));
	State.SetSelectedActor(SelectedActor ? SelectedActor->GetUUID() : static_cast<uint32>(-1));
}

void FEditor::LoadState()
{
	SEditorViewport* Viewport = GetPerspectiveViewport();
	if (!Viewport)
	{
		return;
	}

	FCamera& Camera = Viewport->GetClient().GetViewportCamera();

	Camera.SetPosition(State.GetCameraLocation());
	Camera.SetRotation(State.GetCameraPitch(), State.GetCameraYaw());
	Camera.SetFOV(State.GetCameraFOV());
	Viewport->GetClient().GetGrid().SetCellSize(State.GetGridCellSize());
	Gizmo.Mode = static_cast<EGizmoMode>(State.GetGizmoMode());
	Gizmo.SetGizmoSpace(static_cast<EGizmoSpace>(State.GetGizmoSpace()));

	ViewportLayout.SetSplitterRatio(State.GetSplitter());
}

bool FEditor::RequestStartPIE(const FRequestPlaySessionParams& Params)
{
	if (!PlayManager)
	{
		return false;
	}

	FRequestPlaySessionParams ResolvedParams = Params;

	if (ResolvedParams.DestinationViewportIndex == -1)
	{
		SEditorViewport* ActiveViewport = GetActiveViewport();

		for (int32 Index = 0; Index < static_cast<int32>(MAX_VIEWPORT_COUNT); ++Index)
		{
			if (&ViewportLayout.Viewports[Index] == ActiveViewport)
			{
				ResolvedParams.DestinationViewportIndex = Index;
				break;
			}
		}
	}

	const int32 Index = ResolvedParams.DestinationViewportIndex;

	if (Index < 0 || Index >= static_cast<int32>(MAX_VIEWPORT_COUNT))
	{
		return false;
	}

	if (!ViewportLayout.Viewports[Index].bVisible)
	{
		return false;
	}

	return PlayManager->RequestStartPIE(ResolvedParams);
}

void FEditor::RequestEndPIE()
{
	if (PlayManager)
	{
		PlayManager->RequestEndPIE();
	}
}

void FEditor::ProcessPIERequests()
{
	if (PlayManager)
	{
		PlayManager->ProcessRequests();
	}
}

void FEditor::NewScene()
{
	UnSelectActor();
	Globals::Editor->OpenEmptyLevel(EWorldType::Editor);
	const FEditorState::SplitViewMode SplitMode = State.GetSplitMode();
	State.ResetToDefaults();
	State.SetSplitMode(SplitMode);
	LoadState();
}

void FEditor::SaveScene(const FString& Path)
{
	Globals::Editor->SaveLevel(Path, GetCurrentLevel());
}

void FEditor::LoadScene(const FString& Path)
{
	// 씬 로드
	SEditorViewport* Viewport = GetPerspectiveViewport();
	Globals::Editor->OpenLevel(Path, EWorldType::Editor);
	SelectedActor = nullptr;

	// 로드된 컴포넌트는 대기열에만 쌓이므로, 트랜스폼이 모두 설정된 지금 트리를 만든다.
	ULevel* Level = GetCurrentLevel();
	Level->GetSceneBVH().Build(Level->GetRenderComponents());
}

// 포커스된 뷰포트를 반환
SEditorViewport* FEditor::GetActiveViewport()
{
	return ViewportLayout.ActiveViewport;
}

// Viewports 배열에 원근 뷰포트가 있으면 그걸 반환. 없으면 Active 뷰포트를 반환
// 주로 save, load에 쓰임
SEditorViewport* FEditor::GetPerspectiveViewport()
{
	SEditorViewport* HiddenPerspective = nullptr;
	for (SEditorViewport& EditorViewport : ViewportLayout.Viewports)
	{
		if (EditorViewport.GetClient().GetCameraMode() != ECameraMode::PERSPECTIVE)
		{
			continue;
		}
		if (EditorViewport.bVisible)
		{
			return &EditorViewport;
		}
		if (!HiddenPerspective) //원근 뷰포트를 찾음
		{
			HiddenPerspective = &EditorViewport;
		}
	}
	return HiddenPerspective ? HiddenPerspective : GetActiveViewport();
}

bool FEditor::SelectActor(AActor* Actor)
{
	if (SelectedActor)
	{
		UnSelectActor();
	}

	SelectedActor = Actor;
	if (SelectedActor)
	{
		SelectedTransform = SelectedActor->GetTransform();
		SelectedEulerDegDisplay = SelectedTransform.GetRotation().GetEulerXYZ();
		if (Gizmo.Mode == EGizmoMode::None)
		{
			Gizmo.Mode = EGizmoMode::Translate;
		}

		if (SelectedActorTextComp)
		{
			SelectedActorTextComp->AttachToComponent(SelectedActor.Get()->GetRootComponent());
			FTransform RelativeTrans;
			RelativeTrans.SetLocation(FVector{ 0.0f, 0.0f, 1.5f });
			SelectedActorTextComp->SetRelativeTransform(RelativeTrans);
			SelectedActorTextComp->SetText(L"UUID : " + std::to_wstring(SelectedActor->GetUUID()));
		}
	}

	return true;
}

void FEditor::UnSelectActor()
{
	if (SelectedActor)
	{
		SelectedActor->SetTransform(SelectedTransform);
	}

	SelectedActor = nullptr;

	if (SelectedActorTextComp)
	{
		SelectedActorTextComp->DetachFromComponent();
	}
}

const TArray<UPrimitiveComponent*>& FEditor::GetPrimitiveComponents() const
{
	static const TArray<UPrimitiveComponent*> Empty;
	return GetCurrentLevel()->GetRenderComponents();
}

void FEditor::ClearSelectionForGC()
{
	SelectedActor = nullptr;
	Gizmo.EndInteraction();
	Gizmo.HoveredHandle = EGizmoHandle::None;
}

UWorld* FEditor::GetCurrentWorld() const
{
	return Globals::Editor->GetEditorWorld();
}

ULevel* FEditor::GetCurrentLevel() const
{
	return GetCurrentWorld()->GetCurrentLevel();
}

void FEditor::SpawnActorToCurrentScene(UClass* Type, int Size)
{
	if (Size <= 0)
	{
		return;
	}

	const float Min = State.GetSpawnActorMinLocation();
	const float Max = State.GetSpawnActorMaxLocation();
	if (Min > Max)
	{
		return;
	}

	for (int i = 0; i < Size; ++i)
	{
		FVector Location{
			Random::GetFloat(Min, Max, 2),
			Random::GetFloat(Min, Max, 2),
			Random::GetFloat(Min, Max, 2),
		};

		AActor* NewActor = GetCurrentWorld()->SpawnActor(Type);
		if (!NewActor)
		{
			return;
		}

		FTransform CurrentTransform = NewActor->GetTransform();
		CurrentTransform.SetLocation(Location);
		CurrentTransform.SetScale3D(FVector{ 0.5f, 0.5f, 0.5f });
		NewActor->SetTransform(CurrentTransform);

		// 액터 선택
		SelectActor(NewActor);
	}

	FSceneBVH& BVH = GetCurrentLevel()->GetSceneBVH();
	if (BVH.ShouldRebuild())
	{
		BVH.Build(GetCurrentLevel()->GetRenderComponents());
	}
}

// Split View Mode가 바뀌었을때 ViewLayout을 그에 맞게 하드코딩된 기본값으로 업데이트함
void FEditor::SetViewLayout(FEditorState::SplitViewMode Mode)
{
	ViewportLayout.Rearrange(Mode);

	auto SetCameraMode = [this](int32 ViewportIndex, ECameraMode Mode)
	{
		GetViewportLayout().Viewports[ViewportIndex].GetClient().SetCameraMode(Mode);
	};

	switch (Mode)
	{
	case FEditorState::SplitViewMode::SINGLE:
		SetCameraMode(0, ECameraMode::PERSPECTIVE);
		State.SetSplitMode(FEditorState::SplitViewMode::SINGLE);
		break;

	case FEditorState::SplitViewMode::VERTICAL:
		SetCameraMode(0, ECameraMode::ORTHOGRAPHIC_TOP);
		SetCameraMode(2, ECameraMode::PERSPECTIVE);
		State.SetSplitMode(FEditorState::SplitViewMode::VERTICAL);
		break;

	case FEditorState::SplitViewMode::HORIZONTAL:
		SetCameraMode(0, ECameraMode::ORTHOGRAPHIC_TOP);
		SetCameraMode(1, ECameraMode::PERSPECTIVE);
		State.SetSplitMode(FEditorState::SplitViewMode::HORIZONTAL);
		break;

	case FEditorState::SplitViewMode::QUAD:
		SetCameraMode(0, ECameraMode::ORTHOGRAPHIC_TOP);
		SetCameraMode(1, ECameraMode::PERSPECTIVE);
		SetCameraMode(2, ECameraMode::ORTHOGRAPHIC_FRONT);
		SetCameraMode(3, ECameraMode::ORTHOGRAPHIC_RIGHT);
		State.SetSplitMode(FEditorState::SplitViewMode::QUAD);
		break;
	}

	ViewportLayout.SetActiveViewport(GetPerspectiveViewport());
}

// FEditorApplication::Render에서 필요한 EditorRenderContext을 만든다
FEditorRenderContext FEditor::GetEditorRenderContext(SEditorViewport& EditorViewport, FVisualizerRegistry* VisualizerRegistry)
{
	FEditorRenderContext EditorRenderContext{
		.SelectedActor = SelectedActor,
		.SelectedPrimitive = nullptr,
		.Grid = &EditorViewport.GetClient().GetGrid(),
		.VisualizerRegistry = VisualizerRegistry,
		.SelectedTransform = SelectedTransform,
		.Gizmo = ObjectSelected() ? &Gizmo : nullptr,
		.TextComp = ObjectSelected() ? SelectedActorTextComp : nullptr,
	};

	if (SelectedActor)
	{
		if (USceneComponent* RootComp = SelectedActor->GetRootComponent())
		{
			EditorRenderContext.SelectedPrimitive = RootComp->Cast<UPrimitiveComponent>();
		}
	}
	return EditorRenderContext;
}
