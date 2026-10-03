#pragma once
#include "Editor/Application/IApplication.h"
#include "Editor/Core/FEditor.h"
#include "Editor/UI/Imgui/FImguiManager.h"
#include "Editor/UI/IEditorWindow.h"
#include <memory>
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Input/FCameraInputController.h"

#include "Editor/Visualizer/FVisualizerRegistry.h"
#include "Runtime/UI/SWindow.h"

class FImguiEditorViewportWindow;

class FEditorApplication final : public IApplication
{
	FEditor Editor;

	USceneManager* SceneManager = nullptr;
	ULevel* CurrentScene = nullptr;

	FImguiManager ImguiManager;

	TArray<std::unique_ptr<IEditorWindow>> EditorWindows;
	FImguiEditorViewportWindow* EditorViewportWindow = nullptr;
	FVisualizerRegistry VisualizerRegistry;

	FRenderView* RenderView = nullptr;

	SWindow EditorViewports;

public:
	FEditorApplication();
	~FEditorApplication() override;

	static FEditorApplication& Get()
	{
		static FEditorApplication Instance;
		return Instance;
	}

	FEditorApplication(const FEditorApplication&) = delete;
	FEditorApplication& operator=(const FEditorApplication&) = delete;

	FEditorApplication(FEditorApplication&&) = delete;
	FEditorApplication& operator=(FEditorApplication&&) = delete;

	void Initialize_ImguiWin32DX11(HWND& Window, ID3D11Device* Device, ID3D11DeviceContext* Context);
	void Initialize_Runtime(USceneManager* SceneManager, FRenderView* RenderView);
	void Shutdown() override;
	void Update(float DeltaTime) override;
	void Render() override;
	void OnWindowSize(UINT Width, UINT Height) override;

	void ExecuteCommand(const char* Command);

private:
	void BeginFrame();
	void Tick(float DeltaTime);
};
