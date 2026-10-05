#include "FObjViewerEngine.h"

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

#include <Windows.h>

IMPLEMENT_UCLASS(UObjViewerEngine, UEngine)

void UObjViewerEngine::Init(FEngineLoop* InEngineLoop)
{
	UEngine::Init(InEngineLoop);
	
	//ID3D11Device* Device = nullptr;
	//ID3D11DeviceContext* Context = nullptr;
	//Renderer.GetDeviceAndContext_ImplDX11(Device, Context);

	//TUniquePtr<FObjViewerApplication> ObjViewer = MakeUnique<FObjViewerApplication>(Renderer);
	//ObjViewer->Initialize(Window, Device, Context);
}

void UObjViewerEngine::Tick(float DeltaTime)
{
	//FStatsManager::Get().ResetFrame();
	//FInputManager::Get().BeginFrame();

	//if (Globals::bIsRequestingResize)
	//{
	//	Application->OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);
	//}

	//{
	//	SCOPE_CYCLE_COUNTER("Game");

	//	for (const auto& WorldContext : WorldList)
	//	{
	//		WorldContext.World->Tick(DeltaTime);
	//	}

	//	Application->Update(DeltaTime);
	//}

	//{
	//	SCOPE_CYCLE_COUNTER("Draw");
	//	Renderer.BeginFrame();
	//	Application->Render();
	//	Renderer.SwapBuffer();
	//}

	//SET_CYCLE_COUNTER("Frame", FTimeManager::GetDeltaTime() * 1000.0f);

	//FInputManager::Get().EndFrame();

	//// 입력 메시지 수신 ~ 프레임 종료까지의 지연. 프레임의 맨 마지막이어야 한다.
	//FInputLatencyTimer::Get().Tick();
}

void UObjViewerEngine::Exit()
{
	//Application->Shutdown();
	//SceneManager.Release();

	UEngine::Exit();
}
