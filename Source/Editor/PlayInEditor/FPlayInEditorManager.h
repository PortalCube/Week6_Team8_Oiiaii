#pragma once
#include "Runtime/Engine/Types/IntTypes.h"
#include "PlayInEditorDataTypes.h"
#include <optional>

// 씬 복제 요청, 플레이 시작·종료, 실행 대상 전환
class FEditor;
class UEditorEngine;

//
// Play 요청
// Editor World 복제(duplicate) 요청
// 
// PIE 종료 및 원래 Editor World 복귀 요청
// 
// 씬 종료는 Scene Manager에서 종료한다.


class PIEManager final
{
public:
	PIEManager(UEditorEngine& inEngine) : engine(inEngine)
	{

	}

	bool RequestStartPIE(const FRequestPlaySessionParams& inParams);
	void ProcessRequests();

	void RequestEndPIE();

	void PasuePIE();
	void ResumePIE();

	void Tick(float DeltaTime);

	void Shutdown();

	bool SetViewport(const FRequestPlaySessionParams& Params);

	bool IsPlaySessionInProgress() const
	{
		return State != EPIESessionState::Stopped;
	}

	EPIESessionState GetState() const
	{
		return State;
	}

	/*std::optional<uint64> GetCreationRequestId() const
	{
		return CreationRequestId;
	}*/

	bool IsPIEViewport(int32 ViewportIndex) const;

	/*
	UScene* GetSceneForViewport(int32 ViewportIndex) const;
	FCamera* GetPlayCamera(int32 ViewportIndex);
	*/

private:
	void StartPIE(const FRequestPlaySessionParams& Params);
	void EndPIE();

	void SetViewport();

	bool IsPlaySessionInProgress();

	UEditorEngine& engine;

	EWorldType currentWorldType = EWorldType::Editor;
	EPIESessionState State = EPIESessionState::Stopped;
	FPlayInEditorViewportInfo ViewportInfo;

	std::optional<FRequestPlaySessionParams> PendingStart;
	std::optional<uint64> CreationRequestId;

	bool bPendingEnd = false;
};
