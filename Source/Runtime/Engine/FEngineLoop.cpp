#include "FEngineLoop.h"

#include "Runtime/Core/Globals.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/UEngine.h"
#include "Runtime/Asset/UPackage.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#if defined(ENGINE_OBJECTVIEWER)
#include "Editor/Engine/UObjViewerEngine.h"
#else
#include "Editor/Engine/UEditorEngine.h"
#endif

#include <format>
#include <Windows.h>

void FEngineLoop::Init(HINSTANCE Instance)
{
	// 윈도우 객체 초기화
	WindowsApplication = MakeUnique<FWindowsApplication>(Instance);
	WindowsApplication->CreateMainWindow({
	    .Instance = Instance,
	    .ClassName = Globals::EngineWindowClass,
	    .WindowName = Globals::EngineName,
	    .Width = Globals::WindowWidth,
	    .Height = Globals::WindowHeight,
	});

	// UObject 시스템 초기화

	// TransientPackage 생성
	Globals::TransientPackage = NewObject<UPackage>(nullptr);
	
	// 엔진 객체 초기화
#if defined(ENGINE_OBJECTVIEWER)
	Engine = NewObject<UObjViewerEngine>(GetTransientPackage());
#else
	Engine = NewObject<UEditorEngine>(GetTransientPackage());
#endif

	Engine->Init(this);
}

void FEngineLoop::Tick()
{
	while (!Globals::bIsRequestingExit)
	{
		// 윈도우 종료 메시지 체크
		if (WindowsApplication->CheckExitMessage())
		{
			Globals::bIsRequestingExit = true;
			continue;
		}

		// 시간 업데이트
		FTimeManager::Update();
		Engine->Tick(FTimeManager::GetDeltaTime());
	}
}

void FEngineLoop::Exit()
{
	// 엔진 종료
	Engine->Exit();

	// 윈도우 객체 종료
	WindowsApplication->Quit();
}

HWND FEngineLoop::GetMainWindowHandle() const
{
	const FWindow* MainWindow = GetMainWindow();

	if (!MainWindow)
	{
		return nullptr;
	}

	return MainWindow->GetHandle();
}

FWindow* FEngineLoop::GetMainWindow() const
{
	if (!WindowsApplication)
	{
		return nullptr;
	}

	return WindowsApplication->GetMainWindow();
}
