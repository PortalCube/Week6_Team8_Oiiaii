#include "UEditorEngine.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Core/FMemory.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FEngineLoop.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Utility/FileUtil.h"

#include "Editor/UI/Imgui/FImguiPropertyWindow.h"
#include "Editor/UI/Imgui/FImguiControlPanelWindow.h"
#include "Editor/UI/Imgui/FImguiContentsDrawer.h"
#include "Editor/UI/Imgui/FImguiWorldOutliner.h"
#include "Editor/UI/Imgui/FImguiEditorViewportWindow.h"
#include "Editor/UI/Imgui/FImguiConsoleWindow.h"
#include "Editor/UI/Imgui/FImguiToolBar.h"

#include <Windows.h>

IMPLEMENT_UCLASS(UEditorEngine, UEngine)

void UEditorEngine::Init(FEngineLoop* InEngineLoop)
{
	UEngine::Init(InEngineLoop);

	// 전역 에디터 변수 세팅
	Globals::Editor = this;

	// Device와 Context 가져오기
	// TODO: Device와 Context를 래핑하는 클래스 만들기
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* Context = Renderer.GetContext();

	FWindow* Window = EngineLoop->GetMainWindow();
	if (!Window)
	{
		throw EngineUtil::CreateError("[UEditorEngine::Init] MainWindow 객체를 가져오는데 실패했습니다.");
	}

	// 에디터 WorldContext 등록
	FWorldContext EditorWorldContext{
		.World = nullptr,
		.WorldType = EWorldType::Editor,
		.TravelURL = "",
		.bTravelEmptyLevel = true, // 첫 Tick에서 월드 생성
	};

	WorldList.push_back(EditorWorldContext);

	// ImGui 초기화
	ImguiManager.Initialize_ImplWin32DX11(*Window, Device, Context);

	// UI Window 초기화
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
	    [this](const char* Command)
	    { ExecuteCommand(Command); }));
	EditorWindows.push_back(std::make_unique<FImguiContentsDrawer>());

	// Editor 백엔드 초기화
	Editor.Initialize(this);
	Editor.InitMultiViewport(FEditorViewportClient{});
	Editor.LoadState();
	Editor.SetViewLayout(Editor.State.GetSplitMode());
}

void UEditorEngine::Tick(float DeltaTime)
{
	FStatsManager::Get().ResetFrame();
	FInputManager::Get().BeginFrame();

	////////////////////////////////////////////////////////////
	// Window Resize 이벤트 처리
	////////////////////////////////////////////////////////////
	if (Globals::bIsRequestingResize)
	{
		// 렌더러 스왑체인 조정
		Renderer.OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);

		FVector2 ViewportSize{
			static_cast<float>(Globals::ResizeWidth),
			static_cast<float>(Globals::ResizeHeight)
		};

		// 뷰포트 종횡비 갱신
		for (auto& Viewport : Editor.GetViewports())
		{
			const FVector2 SizePixels = Viewport.LengthUV * ViewportSize;
			Viewport.ViewportCamera.SetAspectRatio(SizePixels.X / SizePixels.Y);
		}

		Globals::bIsRequestingResize = false;
	}

	////////////////////////////////////////////////////////////
	// 에디터 로직 Update
	////////////////////////////////////////////////////////////
	{
		SCOPE_CYCLE_COUNTER("Game");

		// ImGui 준비
		ImguiManager.NewFrame();

		// 월드 처리
		for (auto& WorldContext : WorldList)
		{
			// World Travel 체크
			TickWorldTravel(WorldContext, DeltaTime);

			// 월드 틱 실행
			WorldContext.World->Tick(DeltaTime);
		}

		// 모든 윈도우 Tick
		for (const auto& Window : EditorWindows)
		{
			Window->Process(Editor, DeltaTime);
		}

		// 에디터 Tick
		Editor.Process();

	}


	////////////////////////////////////////////////////////////
	// 에디터 Render
	////////////////////////////////////////////////////////////
	{
		SCOPE_CYCLE_COUNTER("Draw");

		Renderer.BeginFrame();

		TArray<FEditorViewportClient>& EditorViewports = Editor.GetViewports();

		// 렌더 준비
		RenderView.PrepareRender();

		// Active인 ViewportClient만 렌더링
		for (SWindow& Leaf : Editor.Leaf)
		{
			if (!Leaf.bisActive)
			{
				continue;
			}

			FEditorViewportClient& EditorViewport = EditorViewports[Leaf.ViewportIndex];

			// 뷰포트 렌더링 명세 구성
			FSceneView SceneView{
				.Camera = EditorViewport.ViewportCamera,
				.ViewProj = EditorViewport.ViewportCamera.GetViewProjectionMatrix(),
				.TopLeftUV = EditorViewport.TopLeftUV,
				.LengthUV = EditorViewport.LengthUV,
				.ViewMode = EditorViewport.ViewMode,
				.ShowFlags = EditorViewport.ShowFlags,
				.LightConstants = Editor.GlobalLight
			};

			// 에디터 렌더링 컨텍스트 구성
			FEditorRenderContext EditorCtx
			{
				.SelectedActor = Editor.GetSelectedActor(),
				.Grid = &EditorViewport.GetGrid(),
				.VisualizerRegistry = &VisualizerRegistry,
				.SelectedTransform = Editor.SelectedTransform,
				.Gizmo = Editor.ObjectSelected() ? &Editor.GetGizmo() : nullptr,
				.TextComp = Editor.ObjectSelected() ? Editor.GetTextcomp() : nullptr,
			};

			// 선택된 항목 있으면 EditorRenderContext에 넣기. (아웃라인 그리기용)
			// TODO: 얘도 이렇게 넣지 말고 "이런걸 그려라" 라는 식으로 바꿀 것
			if (EditorCtx.SelectedActor)
			{
				if (USceneComponent* RootComp = EditorCtx.SelectedActor->GetRootComponent())
				{
					EditorCtx.SelectedPrimitive = RootComp->Cast<UPrimitiveComponent>();
				}
			}

			// 뷰포트 렌더링 일괄 수행
			// TODO: Level이 여기 들어가면 안됨. 이미 만든 리스트만 받아올 수 있도록 리팩토링
			RenderView.RenderView(SceneView, *WorldList[0].World->GetCurrentLevel(), EditorCtx);
		}

		// 선택된 액터가 있다면 추가 그리기 수행
		if (Editor.ObjectSelected())
		{
			for (const SWindow& Leaf : Editor.Leaf)
			{
				if (!Leaf.bisActive)
				{
					continue;
				}

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

				// 선택된 액터의 UUID 표시 그리기
				RenderView.RenderOverlayPass(Viewport.ViewportCamera, SceneView, Editor.SelectedTransform, Editor.GetGizmo(), Editor.GetTextcomp());

				// 마지막으로 그린 뷰의 렌더 모드가 남지 않도록 설정
				RenderView.SetRenderMode(Viewport.ViewMode);

				// 기즈모 그리기
				RenderView.RenderGizmo(
				    Editor.SelectedTransform,
				    Viewport.ViewportCamera,
				    Viewport.TopLeftUV,
				    Viewport.LengthUV,
				    Editor.GetGizmo());
			}
		}

		// ImGui 라이브러리 렌더링 수행
		ImguiManager.RenderUI();

		// 최종적으로 그리기
		Renderer.SwapBuffer();
	}

	SET_CYCLE_COUNTER("Frame", FTimeManager::GetDeltaTime() * 1000.0f);

	FInputManager::Get().EndFrame();

	// 입력 메시지 수신 ~ 프레임 종료까지의 지연. 프레임의 맨 마지막이어야 한다.
	FInputLatencyTimer::Get().Tick();
}

void UEditorEngine::Exit()
{
	Editor.Shutdown();
	Renderer.Shutdown();

	UEngine::Exit();
}

void UEditorEngine::SaveLevel(const FString& Path, ULevel* Level)
{
	// 사실 여기서 하는건 레벨을 저장한다기 보단 엔진의 스냅샷을 저장하는 것에 가까움
	// NextUUID 같은건 저장해선 안됨. 액터의 고유키나 참조 관계는 핸들로 저장해야함

	FArchive Archive;

	Archive.SetInt32("Version", 1);
	Archive.SetInt32("NextUUID", FUObjectArray::Get().GetNextUUID());

	FArchive LevelArchive;
	Level->Serialize(LevelArchive);
	Archive.SetArchive("Level", LevelArchive);

	FileUtil::WriteArchive(Path, LevelArchive);
}

UWorld* UEditorEngine::GetEditorWorld() const
{
	// 대충 임시
	return WorldList[0].World;
}

void UEditorEngine::ExecuteCommand(const char* Command)
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
