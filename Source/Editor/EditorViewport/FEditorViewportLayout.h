#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/UI/SWindow.h"
#include "Runtime/UI/SSplitter.h"
#include "SEditorViewport.h"
#include "Editor/Core/FEditorState.h"
#include "Runtime/Engine/Showflags.h"

constexpr uint32 MAX_VIEWPORT_COUNT = 4;

struct FEditorViewportLayout
{
	SWindow* Root = nullptr;						// 윈도우를 관리하는 SWindow 포인터. Viewports 중 하나를 가르킴
	SEditorViewport* ActiveViewport = nullptr;		// 현재 포커스된 Viewports를 가르킴
	SEditorViewport* MaximizedViewport = nullptr;	// 현재 최대화된 Viewports를 가르킴
	SEditorViewport Viewports[MAX_VIEWPORT_COUNT];	// Viewport 들을 가지고 있는 배열
	SSplitterH SplitterH1; // 세로선1
	SSplitterH SplitterH2; // 세로선2
	SSplitterV SplitterV; // 가로선

	FEditorViewportLayout() = default;

	void Rearrange(FEditorState::SplitViewMode Mode);
	void Resize(const FRect& InRect);
	void SetActiveViewport(SEditorViewport* InViewport);
	void ToggleMaximize(SEditorViewport* InViewport, FEditorState::SplitViewMode Mode);

	void SetSplitterRatio(FVector InSplitter);
	FVector GetSplitterRatio() const;
};
