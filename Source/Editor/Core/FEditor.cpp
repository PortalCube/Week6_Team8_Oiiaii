#include "FEditor.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/ACubeActor.h"
#include "Runtime/Actors/ASphereActor.h"
#include "Runtime/Actors/ACylinderActor.h"
#include "Runtime/Actors/ABillboardActor.h"
#include "Runtime/Actors/ASpotlightActor.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/Random.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include <numbers>
#include <Runtime/Engine/FSceneBVH.h>

void FEditor::Initialize(USceneManager* SceneManager)
{
	State.ReadFromFile();
	Gizmo.Initialize();
	SelectedActorTextComp = NewObject<UTextComponent>();
	if (SelectedActorTextComp)
	{
		SelectedActorTextComp->Initialize();
		SelectedActorTextComp->SetInheritRotation(false);
		FAssetRegistry& Registry = FAssetRegistry::GetInstance();
		SelectedActorTextComp->SetMesh(Registry.Get<UStaticMesh>("#Rect"));
		SelectedActorTextComp->SetMaterial(Registry.Get<UMaterial>("Material/SelectedActor_Text.json"));
		SelectedActorTextComp->SetFont(FName("bazziotf"));
	}
	this->SceneManager = SceneManager;
}

void FEditor::Shutdown()
{
	SaveState();
	State.FlushToFile();
}

FRenderResourceLibrary* FEditor::GetRendererLibrary()
{
	return &FRenderResourceLibrary::Get();
}

void FEditor::Process()
{
	if (FInputManager::Get().IsKeyDown(VK_F11))
	{
		bZenMode = !bZenMode;
	}

	// 씬의 액터 업데이트

	if (FInputManager::Get().IsKeyPressed(VK_DELETE) && SelectedActor)
	{
		AActor* Target = SelectedActor;
		UnSelectActor();
		Target->Destroy();
	}

	if (SceneManager && SceneManager->CurrentScene)
	{
		SceneManager->CurrentScene->Update(FTimeManager::GetDeltaTime());
	}

	if (SelectedActor)
	{
		USceneComponent* Root = SelectedActor->GetRootComponent();
		const bool bChanged = Root && !(Root->GetRelativeTransform() == SelectedTransform);

		SelectedActor->SetTransform(SelectedTransform);

		// Transform이 변경되었을 때만 Refit
		if (bChanged && SceneManager && SceneManager->CurrentScene)
		{
			RefitActorInBVH(SceneManager->CurrentScene->GetSceneBVH(), SelectedActor);
		}
	}

	SaveState();
	State.Tick(FTimeManager::GetDeltaTime());
}

void FEditor::SaveState()
{
	const SViewport* Viewport = GetActiveViewport();
	if (!Viewport)
	{
		return;
	}

	const FCamera& Camera = Viewport->EditorViewport->Client.GetViewportCamera();
	State.SetCameraLocation(Camera.GetPosition());
	State.SetCameraPitch(Camera.GetPitch());
	State.SetCameraYaw(Camera.GetYaw());
	State.SetCameraFOV(Camera.GetProjection().GetFOV());
	State.SetGridCellSize(Viewport->EditorViewport->Client.GetGrid().GetCellSize());
	State.SetGizmoMode(static_cast<uint8>(Gizmo.Mode));
	State.SetGizmoSpace(static_cast<uint8>(Gizmo.GetSpace()));
	State.SetSelectedActor(SelectedActor ? SelectedActor->GetUUID() : static_cast<uint32>(-1));
}

void FEditor::LoadState()
{
	SViewport* Viewport = GetActiveViewport();
	if (!Viewport)
	{
		return;
	}

	FCamera& Camera = Viewport->EditorViewport->Client.GetViewportCamera();

	Camera.SetPosition(State.GetCameraLocation());
	Camera.SetRotation(State.GetCameraPitch(), State.GetCameraYaw());
	Camera.SetFOV(State.GetCameraFOV());
	Viewport->EditorViewport->Client.GetGrid().SetCellSize(State.GetGridCellSize());
	Gizmo.Mode = static_cast<EGizmoMode>(State.GetGizmoMode());
	Gizmo.SetGizmoSpace(static_cast<EGizmoSpace>(State.GetGizmoSpace()));

	// viewmode관련
	ViewportLayout.SetSplitterRatio(State.GetSplitter());
}

void FEditor::NewScene()
{
	UnSelectActor();
	SceneManager->SetScene(NewObject<UScene>());
	State.ResetToDefaults();
	LoadState();
}

void FEditor::SaveScene(const FString& Path)
{
	SceneManager->SaveScene(Path);
}

void FEditor::LoadScene(const FString& Path)
{
	// 씬 로드
	SViewport* Viewport = GetActiveViewport();
	SceneManager->LoadScene(Path, Viewport ? &Viewport->EditorViewport->Client.GetViewportCamera() : nullptr);
	SelectedActor = nullptr;

	// 로드된 컴포넌트는 대기열에만 쌓이므로, 트랜스폼이 모두 설정된 지금 트리를 만든다.
	if (SceneManager->CurrentScene)
	{
		UScene* Scene = SceneManager->CurrentScene;
		Scene->GetSceneBVH().Build(Scene->GetRenderComponents());
	}
}

bool FEditor::CheckSceneExists()
{
	if (SceneManager->CurrentScene == nullptr)
		return false;
	return true;
}

void FEditor::InitViewports()
{
	//for (uint32 i = 0; i < MAX_VIEWPORT_COUNT; ++i)
	//{
	//	Viewports[i].Client = 

	//}
}

SViewport* FEditor::GetActiveViewport()
{
	return ViewportLayout.ActiveViewport ? ViewportLayout.ActiveViewport : nullptr;
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
			SelectedActorTextComp->SetActorOwner(SelectedActor.Get());
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
		SelectedActorTextComp->SetActorOwner(nullptr);
	}
}

const TArray<UPrimitiveComponent*>& FEditor::GetPrimitiveComponents() const
{
	static const TArray<UPrimitiveComponent*> Empty;
	if (!SceneManager || !SceneManager->CurrentScene)
	{
		return Empty;
	}
	return SceneManager->CurrentScene->GetRenderComponents();
}

void FEditor::ClearSelectionForGC()
{
	SelectedActor = nullptr;
	Gizmo.EndInteraction();
	Gizmo.HoveredHandle = EGizmoHandle::None;
}

void FEditor::SpawnActorToCurrentScene(UClass* Type, int Size)
{
	if (!SceneManager || !SceneManager->CurrentScene)
	{
		return;
	}

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

		AActor* NewActor = SceneManager->CurrentScene->SpawnActor(Type);
		if (!NewActor)
		{
			return;
		}

		FTransform CurrentTransform = NewActor->GetTransform();
		CurrentTransform.SetLocation(Location);
		CurrentTransform.SetScale3D(FVector{ 0.5f, 0.5f, 0.5f });
		NewActor->SetTransform(CurrentTransform);

		// 액터 시작 및 선택
		NewActor->BeginPlay();
		SelectActor(NewActor);
	}

	FSceneBVH& BVH = SceneManager->CurrentScene->GetSceneBVH();
	if (BVH.ShouldRebuild())
	{
		BVH.Build(SceneManager->CurrentScene->GetRenderComponents());
	}
}

void FEditor::ResizeView(FEditorState::SplitViewMode Mode)
{
	ViewportLayout.ResizeLayout(Mode);
}

void FEditor::SetViewLayout(FEditorState::SplitViewMode Mode)
{
	ResizeView(Mode);

	auto SetCameraMode = [this](int32 ViewportIndex, ECameraMode Mode)
	{
		Viewports[ViewportIndex].Client.SetCameraMode(Mode);
	};

	// TODO: 지금은 하드 코딩이지만 나중에 각 뷰포트 마다 값을 변경할 수 있도록
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
	//ViewportLayout.SwitchSplitMode(Mode);
}
