#pragma once

#include "Runtime/Engine/UEngine.h"
#include "Editor/UI/IEditorWindow.h"
#include "Editor/UI/Imgui/FImguiEditorViewportWindow.h"
#include "Editor/UI/Imgui/FImguiManager.h"
#include "Editor/Visualizer/FVisualizerRegistry.h"

class FEngineLoop;
struct FRequestPlaySessionParams;

class UEditorEngine : public UEngine
{
	DECLARE_UCLASS(UEditorEngine, UEngine)

private:

	// TODO: UEditorSubsystem으로 분리
	// 에디터 백엔드 클래스
	FEditor Editor;

	// 에디터 ImGui 윈도우
	TArray<std::unique_ptr<IEditorWindow>> EditorWindows;

	// TODO: UObject 상속으로 바꾸면 제거될 예정
	// 뷰포트 윈도우를 저장하는 포인터
	FImguiEditorViewportWindow* EditorViewportWindow = nullptr;

	// TODO: 제거 예정
	FImguiManager ImguiManager;

	// TODO: RenderView로 이동
	FVisualizerRegistry VisualizerRegistry;

	SWindow EditorViewports;

	FWorldContext* EditorWorldContext = nullptr;
	FWorldContext* PIEWorldContext = nullptr;

public:
	virtual void Init(FEngineLoop* InEngineLoop) override;

	virtual void Tick(float DeltaTime) override;

	virtual void Exit() override;

	void SaveLevel(const FString& Path, ULevel* Level);

	// 에디터 월드 가져오기.
	UWorld* GetEditorWorld() const;
	FWorldContext* GetEditorWorldContext() const;

	// PIE 월드 가져오기.
	UWorld* GetPIEWorld() const;
	FWorldContext* GetPIEWorldContext() const;

	// 임시
	void ExecuteCommand(const char* Command);

	virtual void OnWorldLoaded(FWorldContext& Context) override;	
	// World 복제와 대상 뷰포트 연결이 모두 완료되면 true.
	bool StartPIESession(const FRequestPlaySessionParams& Params);
	void StopPIESession();
};
