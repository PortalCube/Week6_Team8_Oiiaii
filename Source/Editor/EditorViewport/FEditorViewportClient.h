#pragma once
#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/FCamera.h"
#include "Editor/Grid/FGrid.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Runtime/Engine/FSceneView.h"

enum class ECameraMode
{
	PERSPECTIVE,
	ORTHOGRAPHIC,
	ORTHOGRAPHIC_TOP,
	ORTHOGRAPHIC_BOTTOM,
	ORTHOGRAPHIC_LEFT,
	ORTHOGRAPHIC_RIGHT,
	ORTHOGRAPHIC_FRONT,
	ORTHOGRAPHIC_BACK,
};

class FEditorViewportClient final
{
public:
	//======Getter & Setter======

	ECameraMode GetCameraMode() const { return CameraMode; }
	void SetCameraMode(ECameraMode Mode); 
	
	EViewModeIndex const GetViewMode() const { return ViewMode; }
	void SetViewMode(EViewModeIndex InViewMode) { ViewMode = InViewMode; }

	FViewport& GetViewport() { return *Viewport; }
	const FViewport& GetViewport() const { return *Viewport; }
	void SetViewPort(FViewport* InViewport) { Viewport = InViewport; }

	FCamera& GetViewportCamera() { return ViewportCamera; }
	const FCamera& GetViewportCamera() const { return ViewportCamera; }
	void SetViewportCamera(FCamera InViewportCamera) { ViewportCamera = InViewportCamera; }

	class FWorldContext* GetWorldContext() { return WorldContext; }
	void SetWorldContext(FWorldContext* Context);

	FGrid& GetGrid() { return Grid; }
	const FGrid& GetGrid() const { return Grid; }

	bool IsFocused() const { return bFocused; }
	bool IsHovered() const { return bHovered; }

	const uint64 GetShowFlags() const { return ShowFlags; }
	bool HasShowFlag(EEngineShowFlags Flag) const { return (ShowFlags & static_cast<uint64>(Flag)) != 0; }

	//======Getter & Setter======

	void ToggleShowFlag(EEngineShowFlags Flag) { ShowFlags ^= static_cast<uint64>(Flag); }

	void UpdateFocusedAndHovered(bool bFocused, bool bHovered);

	void Update();

	FSceneView GetSceneView(const FLightConstants& InLightConstants);

private:
	bool bFocused = false;
	bool bHovered = false;

	// 뷰포트
	FViewport* Viewport = nullptr;
	
	// 뷰포트 카메라
	FCamera ViewportCamera;

	// 월드 컨텍스트
	FWorldContext* WorldContext;

	// 뷰포트 카메라 모드
	ECameraMode CameraMode = ECameraMode::PERSPECTIVE;
	
	// 뷰포트 뷰 모드
	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;

	// 쇼 플래그
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
	                   static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
	                   static_cast<uint64>(EEngineShowFlags::SF_Grid) |
	                   static_cast<uint64>(EEngineShowFlags::SF_Fog);
	// 그리드
	FGrid Grid;
};
