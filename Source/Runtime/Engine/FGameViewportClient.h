#pragma once

#include "Runtime/Math/FVector2.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Engine/FViewport.h"
#include "Runtime/Engine/ShowFlags.h"
#include "Editor/Grid/FGrid.h"
#include <Editor/EditorViewport/FEditorViewportClient.h>


struct FWorldContext;
class UWorld;

enum class ECameraMode;

class FGameViewportClient final
{
public:

	ECameraMode GetCameraMode() const { return CameraMode; }
	void SetCameraMode(ECameraMode Mode);

	EViewModeIndex const GetViewMode() const { return ViewMode; }
	void SetViewMode(EViewModeIndex InViewMode) { ViewMode = InViewMode; }

	FViewport& GetViewport() { return *Viewport; }
	const FViewport& GetViewport() const { return *Viewport; }
	void SetViewPort(FViewport* InViewport)	{ Viewport = InViewport; }

	FCamera& GetViewportCamera(){ return ViewportCamera;}
	const FCamera& GetViewportCamera() const { return ViewportCamera; }
	void SetViewportCamera(const FCamera& InCamera) { ViewportCamera = InCamera; }

	FWorldContext* GetWorldContext() const { return WorldContext; }
	void SetWorldContext(FWorldContext* InWorldContext);

	FGrid& GetGrid() { return Grid; }
	const FGrid& GetGrid() const { return Grid; }

	// Focus / Hover
	bool IsFocused() const { return bFocused; }
	bool IsHovered() const { return bHovered; }

	// Show flags
	uint64 GetShowFlags() const	{ return ShowFlags; }
	bool HasShowFlag(EEngineShowFlags Flag) const { return (ShowFlags & static_cast<uint64>(Flag)) != 0; }

	void ToggleShowFlag(EEngineShowFlags Flag) { ShowFlags ^= static_cast<uint64>(Flag); }

	void UpdateFocusedAndHovered(bool bInFocused, bool bInHovered);

	void Update();

	// Rendering
	FSceneView GetSceneView(const FLightConstants& InLightConstants);

private:
	bool bFocused = false;
	bool bHovered = false;

	FViewport* Viewport = nullptr;

	FCamera ViewportCamera;

	FWorldContext* WorldContext = nullptr;

	ECameraMode CameraMode = ECameraMode::PERSPECTIVE;

	EViewModeIndex ViewMode = EViewModeIndex::VMI_Lit;

		// 쇼 플래그
	uint64 ShowFlags = static_cast<uint64>(EEngineShowFlags::SF_Primitives) |
	                   static_cast<uint64>(EEngineShowFlags::SF_BillboardText) |
	                   static_cast<uint64>(EEngineShowFlags::SF_Grid);

	// 그리드
	FGrid Grid;
};
