#include "FEditorApplication.h"
#include "Editor/UI/Imgui/FImguiPropertyWindow.h"
#include "Editor/UI/Imgui/FImguiControlPanelWindow.h"
#include "Editor/UI/Imgui/FImguiContentsDrawer.h"
#include "Editor/UI/Imgui/FImguiWorldOutliner.h"
#include "Editor/UI/Imgui/FImguiEditorViewportWindow.h"
#include "Editor/UI/Imgui/FImguiConsoleWindow.h"
#include "Editor/UI/Imgui/FImguiToolBar.h"

#include "Runtime/Components/UAnimatedBillboardComp.h"
#include "Runtime/Components/UBillboardComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Components/UPrimitiveComponent.h"
#include "Runtime/Components/USpotLightComponent.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/FMesh.h"
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <utility>

#include "Runtime/Core/FString.h"

#include "Runtime/Engine/FSceneView.h"

#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/TestTextActor.h"

#include "Editor/Visualizer/IVisualizer.h"
#include "Editor/Core/FEditor.h"
#include <Editor/UI/Imgui/FImguiStatsWindow.h>
#include <Runtime/CoreUObject/FStatsManager.h>
#include "Runtime/Core/Globals.h"

FEditorApplication::FEditorApplication()
{
    EditorWindows.push_back(std::make_unique<FImguiToolbar>());

	// TODO: FImguiWindow를 UObject를 상속받도록 구조 리팩토링
	// 지금 방식에서는 어쩔 수 없이 ViewportWindow를 임시로 저장하는게 제일 편함..
    auto ViewportWindow = std::make_unique<FImguiEditorViewportWindow>();
    EditorViewportWindow = ViewportWindow.get();
    EditorWindows.push_back(std::move(ViewportWindow));

    EditorWindows.push_back(std::make_unique<FImguiWorldOutliner>());
    EditorWindows.push_back(std::make_unique<FImguiControlPanelWindow>());
    EditorWindows.push_back(std::make_unique<FImguiPropertyWindow>());
    EditorWindows.push_back(std::make_unique<FImguiConsoleWindow>(
        [this](const char* Command) { ExecuteCommand(Command); }));
    EditorWindows.push_back(std::make_unique<FImguiContentsDrawer>());
}

FEditorApplication::~FEditorApplication() = default;

void FEditorApplication::Initialize_ImguiWin32DX11(
    HWND& Window, ID3D11Device* Device, ID3D11DeviceContext* Context)
{
	ImguiManager.Initialize_ImplWin32DX11(Window, Device, Context);
}

void FEditorApplication::Initialize_Runtime(USceneManager* SceneManager,
    FRenderView* RenderView)
{
	this->RenderView = RenderView;
	this->SceneManager = SceneManager;
	this->CurrentScene = SceneManager->CurrentScene;

	Editor.Initialize(SceneManager);
	Editor.InitMultiViewport(FEditorViewportClient{});
	Editor.LoadState();
	Editor.SetViewLayout(Editor.State.GetSplitMode());
}

void FEditorApplication::Shutdown()
{
	Editor.Shutdown();
}

void FEditorApplication::Update(float DeltaTime)
{
	BeginFrame();
	Tick(DeltaTime);
}

void FEditorApplication::BeginFrame()
{
	ImguiManager.NewFrame();
}

void FEditorApplication::Tick(float DeltaTime)
{
	for (const auto& Window : EditorWindows)
	{
		Window->Process(Editor, DeltaTime);
	}
	Editor.Process();
}

void FEditorApplication::Render()
{
	TArray<FEditorViewportClient>& EditorViewports = Editor.GetViewports();

	// 렌더 준비
	RenderView->PrepareRender();

	{
		// 컬링 준비 시간 기록?
		//
		// 이동한 오브젝트는 월드 AABB 재계산
		SceneManager->CurrentScene->UpdateDirtyBounds();
	}

	// Active인 ViewportClient만 렌더링
	for (SWindow& Leaf : Editor.Leaf)
	{
		if (!Leaf.bisActive)
			continue;
		FEditorViewportClient& EditorViewport = EditorViewports[Leaf.ViewportIndex];

		// 뷰포트 렌더링 명세 구성
		FSceneView sceneview{
			.Camera = EditorViewport.ViewportCamera,
			.ViewProj = EditorViewport.ViewportCamera.GetViewProjectionMatrix(),
			.TopLeftUV = EditorViewport.TopLeftUV,
			.LengthUV = EditorViewport.LengthUV,
			.ViewMode = EditorViewport.ViewMode,
			.ShowFlags = EditorViewport.ShowFlags,
			.LightConstants = Editor.GlobalLight
		};

		// 에디터 렌더링 컨텍스트 구성
		FEditorRenderContext EditorCtx;
		EditorCtx.SelectedActor = Editor.GetSelectedActor();
		EditorCtx.SelectedTransform = Editor.SelectedTransform;
		EditorCtx.Gizmo = Editor.ObjectSelected() ? &Editor.GetGizmo() : nullptr;
		EditorCtx.TextComp = Editor.ObjectSelected() ? Editor.GetTextcomp() : nullptr;
		EditorCtx.Grid = &EditorViewport.GetGrid();
		EditorCtx.VisualizerRegistry = &VisualizerRegistry;

		if (EditorCtx.SelectedActor)
		{
			if (USceneComponent* RootComp = EditorCtx.SelectedActor->GetRootComponent())
			{
				EditorCtx.SelectedPrimitive = RootComp->Cast<UPrimitiveComponent>();
			}
		}

		// 뷰포트 렌더링 일괄 수행
		RenderView->RenderView(sceneview, *SceneManager->CurrentScene, EditorCtx);
	}

	// 기즈모 그리기
	if (Editor.ObjectSelected())
	{
		for (const SWindow& Leaf : Editor.Leaf)
		{
			if (!Leaf.bisActive)
				continue;

			const auto& Viewport = EditorViewports[Leaf.ViewportIndex];

			FSceneView SceneView{
				.Camera = Viewport.ViewportCamera,
				.ViewProj = Viewport.ViewportCamera.GetViewProjectionMatrix(),
				.TopLeftUV = Viewport.TopLeftUV,
				.LengthUV = Viewport.LengthUV,
				.ViewMode = Viewport.ViewMode,
				.ShowFlags = Viewport.ShowFlags,
				.LightConstants = Editor.GlobalLight
			};

			RenderView->RenderOverlayPass(Viewport.ViewportCamera, SceneView, Editor.SelectedTransform, Editor.GetGizmo(), Editor.GetTextcomp());
			// 마지막으로 그린 뷰의 렌더 모드가 남지 않도록 설정

			RenderView->SetRenderMode(Viewport.ViewMode);
			RenderView->RenderGizmo(
			    Editor.SelectedTransform,
			    Viewport.ViewportCamera,
			    Viewport.TopLeftUV,
			    Viewport.LengthUV,
			    Editor.GetGizmo());
		}
	}

	ImguiManager.RenderUI();
}

void FEditorApplication::OnWindowSize(UINT Width, UINT Height)
{
	// 뷰포트 종횡비 갱신
	for (auto& Viewport : Editor.GetViewports())
	{
		const FVector2 SizePixels =
		    Viewport.LengthUV *
		    FVector2{ static_cast<float>(Width), static_cast<float>(Height) };

		auto& Camera = Viewport.ViewportCamera;
		Camera.SetAspectRatio(SizePixels.X / SizePixels.Y);
	}
}

void FEditorApplication::ExecuteCommand(const char* Command)
{
	if (!Command)
		return;

	FString lowerCmd = Command;
	unsigned int NumberArg = 0;

	std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);

	if (lowerCmd.compare("stat memory") == 0)
	{
		UE_LOG("Stat Memory Command is executed!");
		EditorViewportWindow->Toggle(FImguiStatsWindow::EStatsWindow::Memory);
	}

	else if (lowerCmd.compare("stat fps") == 0)
	{
		UE_LOG("Stat FPS Command is executed!");
		EditorViewportWindow->Toggle(FImguiStatsWindow::EStatsWindow::FPS);
	}

	else if (lowerCmd.compare("stat unit") == 0)
	{
		UE_LOG("Stat unit Command is executed!");
		EditorViewportWindow->Toggle(FImguiStatsWindow::EStatsWindow::Unit);
	}

	else if (lowerCmd.compare("stat none") == 0)
	{
		UE_LOG("Stat Window is closed!");
		EditorViewportWindow->SetClose();
	}

	else if (lowerCmd.compare("stat cull") == 0)
	{
		UE_LOG("Stat Cull Command is executed!");
		// EditorViewportWindow->Toggle(FImguiStatsWindow::EStatsWindow::Cull);
	}

	else if (lowerCmd.compare("cull") == 0)
	{
		// 컬링 토글
		Globals::bEnableFrustumCulling = !Globals::bEnableFrustumCulling;
		UE_LOG("Culling : %s", Globals::bEnableFrustumCulling ? "ON" : "OFF");
	}

	else
	{
		UE_LOG("Unknown command: '%s'\n", Command);
		return;
	}
}
