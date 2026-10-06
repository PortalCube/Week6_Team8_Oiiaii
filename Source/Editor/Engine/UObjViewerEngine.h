#pragma once
#include <Windows.h>
#include "Runtime/Engine/UEngine.h"
#include "Runtime/Engine/FCamera.h"

#include "Runtime/Parser/FObjParser.h"
#include "Runtime/Input/FViewerCameraController.h"

class UObjViewerEngine final : public UEngine
{
	DECLARE_UCLASS(UObjViewerEngine, UEngine)

public:
	void Init(FEngineLoop* InEngineLoop) override;
	void Tick(float DeltaTime) override;
	void Render();
	void Exit() override;
	void OpenObj(const char* InPath);
	void ImportBinary(const char* InPath);
	void ExportObjToBinary(const char* OutPath);

	void OpenMtl(const char* InFilePath);

	void OnWindowSize(UINT Width, UINT Height);

private:
	// UI Functions
	void RenderSideBar();
	void RenderConsole();
	void RenderToolbar();
	void RenderUI();

	TArray<FString> ConsoleLog;
	void AddLog(const FString& Message);
	bool PickFile(FString& OutPath, const wchar_t* InFileFilter, LPCWSTR InlpstrDefExt, bool bSave);

	bool bDockLayoutInitialized = false;
	TSharedPtr<FMesh> CurrentMesh = nullptr;

	TArray<FVertexData> Vertices;
	TArray<uint32> Indices;
	TArray<FMeshSection> Sections;
	TArray<FMtlData> MtlDatas;
	uint64 CurrentObjHash = 0;

	TMap<FString, FMtlData> MtlMap;
	TMap<FString, TSharedPtr<FTexture>> TextureMap;

	FCamera Camera;
	FViewerCameraController CameraController;

	FLightConstants Light;
	float LightYaw = 45.0f;
	float LightPitch = -45.0f;

	float BackgroundColor[4] = { 0.15f, 0.15f, 0.22f, 1.0f };
};
