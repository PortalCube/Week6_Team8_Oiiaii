#include "pch.h"
#include "FPlayInEditorManager.h"
#include "Runtime/Engine/FArchive.h"
// #include "Editor/Engine/UEditorEngine.h"



bool PIEManager::RequestStartPIE(const FRequestPlaySessionParams& inParams)
{
	// 생성 대기·실행·종료 중에는 새 시작 요청을 받지 않는다.
	if (State != EPIESessionState::Stopped)
	{
		return false;
	}

	// 현재 지원하는 실행 범위를 확인한다.
	if (inParams.sessionDestination != EPlaySessionDestinationType::InProcess ||
	    inParams.worldType != EPlaySessionWorldType::PlayInEditor)
	{
		return false;
	}

	 // Editor가 요청 시점의 대상 번호를 확정해야 한다.
	if (inParams.DestinationViewportIndex < 0)
	{
		return false;
	}

	PendingStart = inParams;

	CreationRequestId.reset();
	State = EPIESessionState::Starting;

	return true;

	//StartPIE(inParams);

	//// bRunningPIE = true;
}

void PIEManager::ProcessRequests()
{
	if (State != EPIESessionState::Starting ||
	    !PendingStart.has_value())
	{
		return;
	}

	// 엔진 호출 전에 대기 요청을 소비한다.
	const FRequestPlaySessionParams Params = *PendingStart;
	PendingStart.reset();

	// CreationRequestId = engine.RequestCreatePIESession(Params);

	if (!CreationRequestId.has_value())
	{
		State = EPIESessionState::Stopped;
		return;
	}

	// 접수 완료일 뿐이므로 Starting을 유지한다.
	// 다음 단계에서 이 식별자로 생성 결과를 조회한다.
}

void PIEManager::PasuePIE()
{
}

void PIEManager::RequestEndPIE()
{
	/*if (!bRunningPIE)
	{
		return;
	}*/

	EndPIE();
	//bRunningPIE = false;
}

// 객체 복사 요청
void PIEManager::StartPIE(const FRequestPlaySessionParams& inParams)
{
	/*if (sceneManager.CurrentScene == nullptr)
	{
		return;
	}*/

	currentWorldType = EWorldType::PIE;
	FArchive Editor;
	/*sceneManager.CurrentScene->Serialize(Editor);
	sceneManager.PIEScene->Deserialize(Editor);*/


}

// 객체 복사 해제 요청
void PIEManager::EndPIE()
{

	currentWorldType = EWorldType::Editor;
}
