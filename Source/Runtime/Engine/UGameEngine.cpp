#include "UGameEngine.h"

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

IMPLEMENT_UCLASS(UGameEngine, UEngine)

void UGameEngine::Init(FEngineLoop* InEngineLoop)
{
	UEngine::Init(InEngineLoop);

	// TODO
}

void UGameEngine::Tick(float DeltaTime)
{
	// TODO
}

void UGameEngine::Exit()
{
	// TODO

	UEngine::Exit();
}
