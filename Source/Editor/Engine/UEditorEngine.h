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

public:
	virtual void Init(FEngineLoop* InEngineLoop) override;

	virtual void Tick(float DeltaTime) override;

	virtual void Exit() override;

	void SaveLevel(const FString& Path, ULevel* Level);

	// 임시. 나중에 Subsystem 구현하면 아마 자연스럽게 사라질듯
	UWorld* GetEditorWorld() const;

	// 임시
	void ExecuteCommand(const char* Command);

	// World 복제와 대상 뷰포트 연결이 모두 완료되면 true.
	bool StartPIESession(const FRequestPlaySessionParams& Params);
	void StopPIESession();
};
