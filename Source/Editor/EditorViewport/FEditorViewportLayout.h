#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/UI/SWindow.h"
#include "Runtime/UI/SSplitter.h"
#include "SEditorViewport.h"
#include "Editor/Core/FEditorState.h"
#include "Runtime/Engine/Showflags.h"

struct FEditorViewportLayout
{
	SWindow* Root = nullptr;
	SEditorViewport* ActiveViewport = nullptr;
	SEditorViewport* MaximizedViewport = nullptr;
	SEditorViewport Viewports[MAX_VIEWPORT_COUNT];
	SSplitterH SplitterH1; // 세로선1
	SSplitterH SplitterH2; // 세로선2
	SSplitterV SplitterV; // 가로선

	FEditorViewportLayout() = default;
	FEditorViewportLayout(SWindow* InRoot)
	    : Root(InRoot)
	{}

	void Resize(FEditorState::SplitViewMode Mode)
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

	void SetSplitterRatio(FVector InSplitter)
	{
		SplitterV.Ratio = InSplitter.X;
		SplitterH1.Ratio = InSplitter.Y;
		SplitterH2.Ratio = InSplitter.Z;
	}
};
