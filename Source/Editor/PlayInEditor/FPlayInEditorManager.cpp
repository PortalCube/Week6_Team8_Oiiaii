#include "pch.h"
#include "FPlayInEditorManager.h"
#include "Runtime/Engine/Types/EngineTypes.h"
#include "Editor/Engine/UEditorEngine.h"



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

	State = EPIESessionState::Starting;

	return true;
}

void PIEManager::ProcessRequests()
{
	// 시작 대기 중 종료 요청이 들어오면 시작을 취소합니다.
	if (bPendingEnd)
	{
		bPendingEnd = false;
		PendingStart.reset();

		Globals::Editor->StopPIESession();

		State = EPIESessionState::Stopped;
		return;
	}

	if (State != EPIESessionState::Starting || !PendingStart.has_value())
	{
		return;
	}

	// 엔진 호출 전에 대기 요청을 소비한다.
	const FRequestPlaySessionParams Params = *PendingStart;
	PendingStart.reset();

	 const bool bStarted = Globals::Editor->StartPIESession(Params);

	 State = bStarted ? EPIESessionState::Running : EPIESessionState::Stopped;

}

void PIEManager::PasuePIE()
{
	if (State == EPIESessionState::Running)
	{
		State = EPIESessionState::Paused;
	}
}

void PIEManager::ResumePIE()
{
	if (State == EPIESessionState::Paused)
	{
		State = EPIESessionState::Running;
	}
}

void PIEManager::RequestEndPIE()
{
	if (State == EPIESessionState::Stopped)
	{
		return;
	}

	bPendingEnd = true;
	State = EPIESessionState::Stopping;
}

// 객체 복사 요청
void PIEManager::StartPIE(const FRequestPlaySessionParams& inParams)
{
	currentWorldType = EWorldType::PIE;

}

// 객체 복사 해제 요청
void PIEManager::EndPIE()
{

	currentWorldType = EWorldType::Editor;
}
