#pragma once

#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Engine/FWorldContext.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UObject.h"

class FEngineLoop;

// 엔진의 런타임 계층을 담당하는 클래스
// 엔진 로직, 시스템을 처리하는 부분은 여기서 담당
class UEngine : public UObject
{
	DECLARE_UCLASS(UEngine, UObject)

protected:
	FRenderer Renderer;
	FRenderView RenderView{ Renderer };
	FEngineLoop* EngineLoop = nullptr;
	TArray<FWorldContext> WorldList;

public:

	virtual void Init(FEngineLoop* InEngineLoop);

	virtual void Tick(float DeltaTime);

	virtual void Exit();

	// 원래 언리얼은 Engine에서는 OpenLevel의 사용자 구현 없이 순수 내부 구현만 있어야 하는거 같은데,
	// 지금 수준에선 그런식은 너무 복잡해질테니 여기에 간단하게 구현
	void OpenLevel(const FString& Path, EWorldType Type);

	void OpenEmptyLevel(const EWorldType Type);

	// 월드 Travel 구현
	void TickWorldTravel(FWorldContext& Context, float DeltaTime);
};
