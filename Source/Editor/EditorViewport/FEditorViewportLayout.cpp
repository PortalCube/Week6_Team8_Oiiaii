#include "FEditorViewportLayout.h"

// Split View Mode가 바뀌었을때 ViewLayout을 그에 맞게 하드코딩된 기본값으로 업데이트함
void FEditorViewportLayout::Rearrange(FEditorState::SplitViewMode Mode)
{
	// viewport를 가지고있는 splitter,window를 업데이트
	ActiveViewport = &Viewports[0];
	MaximizedViewport = nullptr;
	Root = &Viewports[0];
	//=== 초기화 ===//
	Viewports[0].bVisible = false;
	Viewports[1].bVisible = false;
	Viewports[2].bVisible = false;
	Viewports[3].bVisible = false;

	SplitterH1.bVisible = false;
	SplitterH2.bVisible = false;
	SplitterV.bVisible = false;
	//=== 초기화 ===//

	//===람다함수===//
	auto Connect = [](SSplitter& Splitter, SWindow& LT, SWindow& RB)
	{
		Splitter.SideLT = &LT;
		Splitter.SideRB = &RB;

		Splitter.bVisible = true;
		LT.bVisible = true;
		RB.bVisible = true;
	};

	switch (Mode)
	{
	case FEditorState::SplitViewMode::SINGLE:
		Viewports[0].bVisible = true;
		Root = &Viewports[0];
		break;

	case FEditorState::SplitViewMode::HORIZONTAL:
		Viewports[0].bVisible = true;
		Viewports[1].bVisible = true;
		Connect(SplitterH1, Viewports[0], Viewports[1]);
		Root = &SplitterH1;
		break;

	case FEditorState::SplitViewMode::VERTICAL:
		Viewports[0].bVisible = true;
		Viewports[2].bVisible = true;
		Connect(SplitterV, Viewports[0], Viewports[2]);
		Root = &SplitterV;
		break;

	case FEditorState::SplitViewMode::QUAD:
		Viewports[0].bVisible = true;
		Viewports[1].bVisible = true;
		Viewports[2].bVisible = true;
		Viewports[3].bVisible = true;
		Connect(SplitterV, SplitterH1, SplitterH2);
		Connect(SplitterH1, Viewports[0], Viewports[1]);
		Connect(SplitterH2, Viewports[2], Viewports[3]);
		Root = &SplitterV;
		break;
	}
}

void FEditorViewportLayout::Resize(const FRect& InRect)
{
	if (Root)
	{
		Root->OnResize(InRect);
	}
}

// 보이는 뷰포트만 활성화
void FEditorViewportLayout::SetActiveViewport(SEditorViewport* InViewport)
{
	if (InViewport && InViewport->bVisible)
	{
		ActiveViewport = InViewport;
	}
}

void FEditorViewportLayout::ToggleMaximize(SEditorViewport* InViewport, FEditorState::SplitViewMode Mode)
{
	if (!InViewport || Mode == FEditorState::SplitViewMode::SINGLE)
	{
		return;
	}

	if (MaximizedViewport)
	{
		SEditorViewport* RestoredViewport = MaximizedViewport;
		Rearrange(Mode);
		ActiveViewport = RestoredViewport;
		return;
	}

	if (!InViewport->bVisible)
	{
		return;
	}

	Rearrange(FEditorState::SplitViewMode::SINGLE);
	Viewports[0].bVisible = false;
	InViewport->bVisible = true;
	Root = InViewport;
	MaximizedViewport = InViewport;
	ActiveViewport = InViewport;
}

// 인자로 들어온 Ratio대로 업데이트
void FEditorViewportLayout::SetSplitterRatio(FVector InSplitterRatio)
{
	SplitterV.Ratio = InSplitterRatio.X;
	SplitterH1.Ratio = InSplitterRatio.Y;
	SplitterH2.Ratio = InSplitterRatio.Z;
}

FVector FEditorViewportLayout::GetSplitterRatio() const
{
	return FVector(SplitterV.Ratio, SplitterH1.Ratio, SplitterH2.Ratio);
}
