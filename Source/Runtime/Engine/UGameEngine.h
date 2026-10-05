#pragma once

#include "Runtime/Engine/UEngine.h"
#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Engine/FWorldContext.h"
#include "Runtime/Core/TArray.h"

class FEngineLoop;

class UGameEngine : public UEngine
{
	DECLARE_UCLASS(UGameEngine, UEngine)

public:
	virtual void Init(FEngineLoop* InEngineLoop) override;

	virtual void Tick(float DeltaTime) override;

	virtual void Exit() override;
};
