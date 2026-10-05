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

	FRenderResourceLibrary& RenderResources = FRenderResourceLibrary::Get(); // 로딩 스크린
	if (!RenderResources.Initialize(Renderer))
	{
		throw EngineUtil::CreateError("FRenderResourceLibrary 초기화에 실패했습니다.");
	}

	FResourceLoader::LoadAssets(); // 로딩 스크린
}

// Tick은 각 엔진별 내부 구현
void UEngine::Tick(float DeltaTime) { }

void UEngine::Exit()
{
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

	// 파일에서 불러올지 여부
	// 빈 레벨보다 파일 레벨을 우선함
	bool bLoadFromFile = !Context.TravelURL.empty();
	Context.bTravelEmptyLevel = false;

	// 파일 경로에서 Archive 생성
	// TODO: 여기 FArchive가 아마 3번 만들어질텐데 구조 바꿔야 됨
	FArchive Archive;

	if (bLoadFromFile)
	{
		try
		{
			Archive = FileUtil::ReadArchive(Context.TravelURL);
		}
		catch (...)
		{
			UE_LOG("[OpenLevel] 파일에서 Level을 불러오는데 실패했습니다.");
			Context.TravelURL = "";
			return;
		}

		// URL Flush
		Context.TravelURL = "";

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

	if (Context.World)
	{
		// 기존 월드 파괴 시작

		// TODO: AGameMode의 StartToLeaveMap 실행

		Context.World->EndPlay();
		Context.World->CleanupWorld();
	}


	UWorld* World = NewObject<UWorld>(Globals::Engine);
	ULevel* Level = NewObject<ULevel>(World);
	Level->Initialize();

	if (bLoadFromFile)
	{
		// 레벨 불러오기
		FArchive LevelArchive = Archive.GetArchive("Level");

		Level->Deserialize(LevelArchive);
	}
	
	// World에 할당
	if (Context.World)
	{
		DestroyObject(Context.World);
	}

	Context.World = World;
	World->LoadLevel(Level);
}
