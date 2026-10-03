#include "FEditorApplication.h"

#include <algorithm>
#include <cctype>
#include <Windows.h>

#include "Editor/Core/FEditor.h"
#include "Editor/EditorViewport/SEditorViewport.h"
#include "Editor/UI/Imgui/FImguiStatsWindow.h"
#include "Editor/Visualizer/IVisualizer.h"

#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/TestTextActor.h"
#include "Runtime/Components/UAnimatedBillboardComp.h"
#include "Runtime/Components/UBillboardComponent.h"
#include "Runtime/Components/UPrimitiveComponent.h"
#include "Runtime/Components/USpotLightComponent.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/FRayCastingManager.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Geometry/FAxisAlignedBoundingBox.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Rendering/FMesh.h"

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
	Editor.SetViewLayout(Editor.State.GetSplitMode());
	Editor.LoadState();

	// TEMP: 당분간 기본값으로 활성화
	EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Unit);
	EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::FPS);
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
	ToolBar.Process(Editor, ConsoleWindow, ControlPanelWindow, PropertyWindow);
	EditorViewportWindow.Process(Editor, DeltaTime);
	WorldOutliner.Process(Editor);
	ControlPanelWindow.Process(Editor);
	PropertyWindow.Process(Editor);
	ConsoleWindow.Process(Editor, [this](const char* Command) { ExecuteCommand(Command); });
	ContentsDrawer.Process(Editor);
	Editor.Process();
}

void FEditorApplication::Render()
{
	// 렌더 준비
	RenderView->PrepareRender();

	{
		// 컬링 준비 시간 기록?
		//
		// 이동한 오브젝트는 월드 AABB 재계산
		SceneManager->CurrentScene->UpdateDirtyBounds();
	}

	// Active인 ViewportClient만 렌더링
	for (SEditorViewport& Viewport : Editor.GetViewportLayout().Viewports)
	{
		if (!Viewport.IsRenderable())
			continue;

		// 뷰포트 렌더링 명세 구성
		FSceneView SceneView = Viewport.GetClient().GetSceneView(Editor.GlobalLight);

		// 에디터 렌더링 컨텍스트 구성
		FEditorRenderContext EditorRenderContext = Editor.GetEditorRenderContext(Viewport, &VisualizerRegistry);

		// 뷰포트 렌더링 일괄 수행
		RenderView->RenderView(SceneView, *SceneManager->CurrentScene, EditorRenderContext);
	}

	// 기즈모 그리기
	if (Editor.ObjectSelected())
	{
		for (SEditorViewport& Viewport : Editor.GetViewportLayout().Viewports)
		{
			if (!Viewport.IsRenderable())
				continue;

			FSceneView SceneView = Viewport.GetClient().GetSceneView(Editor.GlobalLight);

			RenderView->RenderOverlayPass(SceneView, Editor.SelectedTransform, Editor.GetGizmo(), Editor.GetTextcomp());

			RenderView->RenderGizmo(
			    Editor.SelectedTransform,
			    Viewport.GetClient().GetViewportCamera(),
			    Viewport.GetClient().GetViewport().GetLeftTop(),
			    Viewport.GetClient().GetViewport().GetRightBottom(),
			    Editor.GetGizmo());
		}
	}

	// ImGui는 마지막에 그림
	RenderView->GetRenderer().BindBackBufferRenderTargets();
	ImguiManager.RenderUI();
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
		EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Memory);
	}

	else if (lowerCmd.compare("stat fps") == 0)
	{
		UE_LOG("Stat FPS Command is executed!");
		EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::FPS);
	}

	else if (lowerCmd.compare("stat unit") == 0)
	{
		UE_LOG("Stat unit Command is executed!");
		EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Unit);
	}

	else if (lowerCmd.compare("stat none") == 0)
	{
		UE_LOG("Stat Window is closed!");
		EditorViewportWindow.SetClose();
	}

	else if (lowerCmd.compare("stat cull") == 0)
	{
		UE_LOG("Stat Cull Command is executed!");
		// EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Cull);
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
