#pragma once

#include "Runtime/Engine/UWorld.h"
#include "Runtime/Engine/Types/EngineTypes.h"

// FWorldContext
// UnrealEngine/Engine/Source/Runtime/Engine/Classes/Engine/Engine.h:351

/*
* UWorld의 포인터를 갖는 객체입니다.
* 언리얼의 표현을 빌리자면, 이 객체는 "월드 트랙"처럼 생각할 수 있습니다.
*
* Game 타입의 WorldContext은 게임 월드를 나타내는 UWorld를 가집니다.
* Editor 타입의 WorldContext는 에디터 월드를 나타내는 UWorld를 가집니다. PIE도 같습니다.
*
* 일부 Level 전환 경로들은 World를 파괴하고 새로 만듭니다. (이것을 Hard Travel이라고 합니다.)
* FWorldContext를 사용하면 UEngine::WorldList를 그대로 냅두고도 월드를 swap할 수 있습니다.
* 
* 또한 Hard Travel 과정에서 유지되어야 하는 객체를 FWorldContext이 소유하여
* World가 파괴될 때 객체들을 그대로 유지해서 다음 World로 넘겨줄 수 있습니다.
*/
struct FWorldContext
{
	UWorld* World;
	EWorldType WorldType;

	/*
	* 다음으로 불러올 레벨을 가리킵니다.
	* 빈칸으로 지정하면 현재 레벨을 유지합니다.
	* 뭔가 적혀있으면 그 경로의 .Scene 파일을 가지고 레벨의 로드를 시도합니다.
	*/
	FString TravelURL;

	// 다음으로 비어있는 레벨을 불러오도록 지정합니다.
	// 단, TravelURL이 있으면 이 옵션은 무시됩니다.
	bool bTravelEmptyLevel;
};
