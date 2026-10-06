#pragma once
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Engine/FCamera.h"
#include "Editor/Core/FEditor.h"
#include <optional>


struct FPlayInEditorViewportInfo
{
	int32 DestinationViewportIndex = -1;
	FCamera PlayCamera;
};

enum class EPIESessionState : uint8
{
	Stopped,		  // 진행 중인 세션과 시작 요청 없음
	Starting,		  // 시작 요청이 접수 되었고, 생성, 연결 진행 중
	Running,		  // 엔진 준비와 대상 뷰포트 연결이 모두 완료
	Paused,			  // 화면 연결을 유지하면서 게임 진행이 정지됨
	Stopping		  // 입력, 화면 연결 해제와 실행 자원 정리 진행 중
};

enum class EPlaySessionDestinationType : uint8
{
	// Editor 프로세스 내부에서 PIE 실행
	InProcess,
	// 새로운 프로세스를 하나 실행. 현재 구현 X
	NewProcess,
	// 별도의 새로운 실행 파일 (언리얼 방식) 현재 구현 X
	Launcher
};

enum class EPlaySessionWorldType : uint8
{
	// PIE 모드
	PlayInEditor,

	// SIE 모드(현재 구현 X)
	SimulateInEditor
};

struct FRequestPlaySessionParams
{
public:
	FRequestPlaySessionParams() : sessionDestination(EPlaySessionDestinationType::InProcess), worldType(EPlaySessionWorldType::PlayInEditor) {};

	EPlaySessionDestinationType sessionDestination;

	EPlaySessionWorldType worldType;

	// -1이면 요청 당시의 활성 뷰포트를 사용한다.
	int32 DestinationViewportIndex = -1;

	// 향후 플레이어 스폰 위치 지정에 사용.
	std::optional<FVector> StartLocation;

	// 프로젝트의 회전 타입을 사용한다.
	std::optional<FQuaternion> StartRotation;

	// 시작 위치를 사용할지 판단
	bool HasPlayWorldPlacement() const
	{
		return worldType == EPlaySessionWorldType::PlayInEditor && StartLocation.has_value();
	}

};
