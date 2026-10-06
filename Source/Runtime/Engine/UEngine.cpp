#include "UEngine.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Core/FMemory.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FEngineLoop.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Utility/EngineUtil.h"
#include "Runtime/Utility/FileUtil.h"

#include <Windows.h>

IMPLEMENT_UCLASS(UEngine, UObject)

void UEngine::Init(FEngineLoop* InEngineLoop)
{
	EngineLoop = InEngineLoop;

	HWND Window = EngineLoop->GetMainWindowHandle();
	if (!Renderer.Initialize(Window))
	{
		throw EngineUtil::CreateError("FRenderer 초기화에 실패했습니다.");
	}
	FStatsManager::Get().Initialize(Renderer.GetDevice());
	FMemory::Init();

	FRenderResourceLibrary& RenderResources = FRenderResourceLibrary::Get(); // TODO: 로딩 스크린 적용
	if (!RenderResources.Initialize(Renderer))
	{
		throw EngineUtil::CreateError("FRenderResourceLibrary 초기화에 실패했습니다.");
	}

	FResourceLoader::LoadAssets(); // TODO: 로딩 스크린 적용
}

// Tick은 각 엔진별 내부 구현
void UEngine::Tick(float DeltaTime) {}

void UEngine::Exit()
{
	// 월드 정리
	for (auto& WorldContext : WorldList)
	{
		WorldContext.World->EndPlay();
		WorldContext.World->CleanupWorld();
		DestroyObject(WorldContext.World);
	}

	Renderer.Shutdown();
}

void UEngine::OpenLevel(const FString& Path, EWorldType Type)
{
	// 전환시킬 WorldContext 찾기

	// 언리얼은 WorldContextObject라는 UObject 객체를 OpenLevel로 넘겨받아 World를 뽑아옴.
	// 우리 엔진은 간단하게 WorldList에서 Type에 맞는 World를 찾는 방식으로 구현
	FWorldContext* WorldContext = nullptr;
	for (auto& Item : WorldList)
	{
		if (Item.WorldType == Type)
		{
			WorldContext = &Item;
		}
	}

	if (WorldContext)
	{
		WorldContext->TravelURL = Path;
		WorldContext->bTravelEmptyLevel = false;
	}
}

void UEngine::OpenEmptyLevel(const EWorldType Type)
{
	FWorldContext* WorldContext = nullptr;
	for (auto& Item : WorldList)
	{
		if (Item.WorldType == Type)
		{
			WorldContext = &Item;
		}
	}

	if (WorldContext)
	{
		WorldContext->TravelURL = "";
		WorldContext->bTravelEmptyLevel = true;
	}
}

void UEngine::TickWorldTravel(FWorldContext& Context, float DeltaTime)
{
	// 월드 Travel이 발생하는지 확인
	if (Context.TravelURL == "" && !Context.bTravelEmptyLevel)
	{
		return;
	}

	// Note: TravelURL과 bTravelEmptyLevel이 동시에 지정되었으면
	// TravelURL대로 불러옴
	LoadMap(Context, Context.TravelURL);

	Context.TravelURL = "";
	Context.bTravelEmptyLevel = false;
}

void UEngine::LoadMap(FWorldContext& Context, const FString& Path)
{
	// 파일 경로에서 Archive 생성
	FArchive Archive;

	if (!Path.empty())
	{

		try
		{
			// TODO: 대입이라 RVO가 발생하질 못해서 FArchive가 2번 만들어지는데 다른 구조로 바꿔야 됨..
			Archive = FileUtil::ReadArchive(Path);
		}
		catch (...)
		{
			UE_LOG("[OpenLevel] 파일에서 Level을 불러오는데 실패했습니다.");
			return;
		}

		// Version 체크
		int32 Version = Archive.GetInt32("Version");
		if (Version != 1)
		{
			UE_LOG("[OpenLevel] 로드하려는 파일의 Level Schema 버전이 다릅니다. 파일의 버전: %d, 지원하는 버전: %d", Version, 1);
			return;
		}

		// UUID 세팅
		int32 NextUUID = Archive.GetInt32("NextUUID");
		FUObjectArray& ObjectArray = FUObjectArray::Get();
		ObjectArray.SetNextUUID(NextUUID);

		// Level 체크
		if (Archive.IsNull("Level"))
		{
			UE_LOG("[OpenLevel] 로드하려는 파일에서 Level 항목이 없습니다. 파일 형식이 올바르지 않습니다.");
			return;
		}
	}

	// 기존 월드 제거
	if (Context.World)
	{
		// TODO: AGameMode의 StartToLeaveMap 실행

		Context.World->EndPlay();
		Context.World->CleanupWorld();
		DestroyObject(Context.World);
	}

	if (!Path.empty())
	{
		// 새로운 월드로 대입
		Context.World = UWorld::CreateWorldWithLevel(Archive, Context.WorldType);
	}
	else
	{
		Context.World = UWorld::CreateWorldWithEmptyLevel(Context.WorldType);
	}
	// 월드 초기화 (Subsystem 및 물리 등록)
	Context.World->InitWorld();

	// Subsystem에 모든 월드 액터/컴포넌트를 등록
	Context.World->UpdateWorldComponents();

	// 액터/컴포넌트들의 상호 초기화 단계
	Context.World->InitializeActorsForPlay();

	// BeginPlay
	Context.World->BeginPlay();
	
	OnWorldLoaded(Context);
}
