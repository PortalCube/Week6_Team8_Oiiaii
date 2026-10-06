#pragma once

#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/Geometry/FTransform.h"

class AActor;
class FGizmo;
class FGrid;
class FVisualizerRegistry;
class UPrimitiveComponent;
class UTextComponent;

// 뷰포트 렌더링 명세
struct FSceneView
{
	const FCamera& Camera;
	FMatrix ViewProj;
	const FViewport& Viewport;
	FVector2 ViewportSizePixel; // 뷰포트의 width, height. 뷰포트는 각자의 RTV를 가지기 때문에 0,0에서부터 시작한다
	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives);
	FLightConstants LightConstants{};
};

// 에디터 렌더링 컨텍스트
struct FEditorRenderContext
{
	const AActor* SelectedActor = nullptr;
	UPrimitiveComponent* SelectedPrimitive = nullptr;
	FGrid* Grid = nullptr;
	FVisualizerRegistry* VisualizerRegistry = nullptr;
	FTransform SelectedTransform;
	const FGizmo* Gizmo = nullptr;
	UTextComponent* TextComp = nullptr;
};
