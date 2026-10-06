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

#include "Runtime/Actors/ACubeActor.h"
#include "Runtime/Components/Mesh/UStaticMeshComponent.h"

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
	WorldList.push_back({
	    .World = nullptr,
	    .WorldType = EWorldType::Editor,
	});

	EditorWorldContext = &WorldList[0];

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
	Editor.SetViewLayout(Editor.State.GetSplitMode());
	Editor.LoadState();

	// 새로운 Level으로 World 불러오기
	LoadMap(*EditorWorldContext, "");
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

		// 렌더 준비
		RenderView.PrepareRender();

		// 컬링 준비 시간 기록? 이동한 오브젝트는 월드 AABB 재계산
		GetEditorWorld()->GetCurrentLevel()->UpdateDirtyBounds();

		// Active인 Viewport 마다 렌더링
		for (SEditorViewport& EditorViewport : Editor.GetViewportLayout().Viewports)
		{
			if (!EditorViewport.IsRenderable())
			{
				continue;
			}

			// 뷰포트 렌더링 명세 구성
			FSceneView View = EditorViewport.GetClient().GetSceneView(Editor.GlobalLight);

			// 뷰포트의 RT를 준비
			if (!RenderView.GetRenderer().PrepareViewportRenderTarget(EditorViewport.GetViewport()))
			{
				continue; // 생성에 실패하면 이번 프레임은 이 뷰포트를 건너뛴다
			}

			// 에디터 렌더링 컨텍스트 구성
			FEditorRenderContext EditorRenderContext = Editor.GetEditorRenderContext(EditorViewport, &VisualizerRegistry);

			// 뷰포트 렌더링 일괄 수행
			RenderView.RenderView(View, *Viewport.GetClient().GetWorldContext()->World->GetCurrentLevel(), EditorRenderContext);

			// 기즈모 그리기
			if (Editor.ObjectSelected())
			{
				RenderView.RenderOverlayPass(View, Editor.SelectedTransform, Editor.GetGizmo(), Editor.GetTextcomp());
				RenderView.RenderGizmo(View, Editor.SelectedTransform, Editor.GetGizmo());
			}

			// 백버퍼 바인딩 후 셰이더로 합성
			RenderView.GetRenderer().CompositeViewport(EditorViewport.GetViewport());

		}

		// ImGui 라이브러리 렌더링 수행
		auto BackBufferRTV = RenderView.GetRenderer().GetBackBufferRTV();
		RenderView.GetRenderer().BindRenderTarget(BackBufferRTV, nullptr);
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

	FileUtil::WriteArchive(Path, Archive);
}

UWorld* UEditorEngine::GetEditorWorld() const
{
	if (!EditorWorldContext)
	{
		return nullptr;
	}

	return EditorWorldContext->World;
}

FWorldContext* UEditorEngine::GetEditorWorldContext() const
{
	return EditorWorldContext;
}

UWorld* UEditorEngine::GetPIEWorld() const
{
	if (!PIEWorldContext)
	{
		return nullptr;
	}

	return PIEWorldContext->World;
}

FWorldContext* UEditorEngine::GetPIEWorldContext() const
{
	return PIEWorldContext;
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

void UEditorEngine::OnWorldLoaded(FWorldContext& Context)
{
	Editor.OnWorldLoaded(Context);
}

void UEditorEngine::StartPIESession()
{
	// 일단 대충 구현
	// 지연된 시작은 조금 나중에 구현

	if (PIEWorldContext)
	{
		EndPIESession();
	}

	// 에디터 WorldContext 등록
	WorldList.push_back({
	    .World = nullptr,
	    .WorldType = EWorldType::PIE,
	});

	PIEWorldContext = &WorldList[1];

	// TODO: Editor World를 복제하기
	// 지금은 비어있는 월드를 생성
	LoadMap(*PIEWorldContext, "");

	// 테스트. 나중에 없애야함
	FVector Location;
	ACubeActor* TestActor1 = PIEWorldContext->World->SpawnActor<ACubeActor>(ACubeActor::StaticClass());
	UStaticMeshComponent* TestMesh1 = TestActor1->GetRootComponent()->Cast<UStaticMeshComponent>();

	Location = {0.0, 5.0f, 0.0f};
	TestMesh1->SetRelativeLocation(Location);

	ACubeActor* TestActor2 = PIEWorldContext->World->SpawnActor<ACubeActor>(ACubeActor::StaticClass());
	UStaticMeshComponent* TestMesh2 = TestActor1->GetRootComponent()->Cast<UStaticMeshComponent>();

	Location = { 0.0, 0.0f, 5.0f };
	TestMesh2->SetRelativeLocation(Location);
}

void UEditorEngine::EndPIESession()
{
	if (PIEWorldContext)
	{
		// TODO: AGameMode의 StartToLeaveMap 실행

		PIEWorldContext->World->EndPlay();
		PIEWorldContext->World->CleanupWorld();
		DestroyObject(PIEWorldContext->World);
	}
}
