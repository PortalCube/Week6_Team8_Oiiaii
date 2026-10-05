#pragma once

#include "Runtime/Engine/UEngine.h"

class FEngineLoop;

// 엔진의 런타임 계층을 담당하는 클래스
// 엔진 로직, 시스템을 처리하는 부분은 여기서 담당
class UObjViewerEngine: public UEngine
{

	DECLARE_UCLASS(UObjViewerEngine, UEngine)

public:
	virtual void Init(FEngineLoop* InEngineLoop) override;

	virtual void Tick(float DeltaTime) override;

	virtual void Exit() override;
};
