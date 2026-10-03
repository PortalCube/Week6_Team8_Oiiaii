#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/UI/SWindow.h"
#include "Runtime/UI/SSplitter.h"
#include "FEditorViewport.h"
#include "Runtime/UI/SViewport.h"
#include "Runtime/Engine/Showflags.h"

struct FEditorViewportLayout
{
	SWindow* Root;
	SViewport* ActiveViewport = nullptr;
	SViewport Leaf[MAX_VIEWPORT_COUNT];
	SSplitterH HorizonSplitter;  // 세로선
	SSplitterH HorizonSplitter2; // 세로선
	SSplitterV VerticalSplitter; // 가로선

	FEditorViewportLayout() = default;
	FEditorViewportLayout(SWindow* InRoot)
	    : Root(InRoot)
	{}

	void Initialize(FEditorViewport*& InEditorViewports)
	{
		for (uint32 i = 0; i < MAX_VIEWPORT_COUNT; ++i)
		{
			Leaf[i].EditorViewport = &InEditorViewports[i];
		}
		ActiveViewport = &Leaf[0];
	}

	void ResizeLayout(FEditorState::SplitViewMode Mode)
	{
		// viewport를 가지고있는 splitter,window를 업데이트
		ActiveViewport = &Leaf[0];
		//=== 초기화 ===//
		Leaf[0].bisActive = false;
		Leaf[1].bisActive = false;
		Leaf[2].bisActive = false;
		Leaf[3].bisActive = false;

		HorizonSplitter2.bisActive = false;
		VerticalSplitter.bisActive = false;
		HorizonSplitter.bisActive = false;
		//=== 초기화 ===//

		//===람다함수===//
		auto Connect = [](SSplitter& Splitter, SWindow& LT, SWindow& RB)
		{
			Splitter.SideLT = &LT;
			Splitter.SideRB = &RB;

			Splitter.bisActive = true;
			LT.bisActive = true;
			RB.bisActive = true;
		};

		switch (Mode)
		{
		case FEditorState::SplitViewMode::SINGLE:
			Leaf[0].bisActive = true;
			Root = &Leaf[0];
			break;

		case FEditorState::SplitViewMode::HORIZONTAL:
			Leaf[0].bisActive = true;
			Leaf[1].bisActive = true;
			Connect(HorizonSplitter, Leaf[0], Leaf[1]);
			Root = &HorizonSplitter;
			break;

		case FEditorState::SplitViewMode::VERTICAL:
			Leaf[0].bisActive = true;
			Leaf[2].bisActive = true;
			Connect(VerticalSplitter, Leaf[0], Leaf[2]);
			Root = &VerticalSplitter;
			break;

		case FEditorState::SplitViewMode::QUAD:
			Leaf[0].bisActive = true;
			Leaf[1].bisActive = true;
			Leaf[2].bisActive = true;
			Leaf[3].bisActive = true;
			Connect(VerticalSplitter, HorizonSplitter, HorizonSplitter2);
			Connect(HorizonSplitter, Leaf[0], Leaf[1]);
			Connect(HorizonSplitter2, Leaf[2], Leaf[3]);
			Root = &VerticalSplitter;
			break;
		}
	}

	void SetSplitterRatio(FVector InSplitter)
	{
		VerticalSplitter.Ratio = InSplitter.X;
		HorizonSplitter.Ratio = InSplitter.Y;
		HorizonSplitter2.Ratio = InSplitter.Z;
	}

	// void SwitchSplitMode(FEditorState::SplitViewMode mode)
	//{
	//	switch (mode)
	//	{
	//	case FEditorState::SplitViewMode::SINGLE:
	//		VerticalSplitter.bisActive = false;
	//		HorizonSplitter.bisActive = false;
	//		HorizonSplitter2.bisActive = false;
	//		break;

	//	case FEditorState::SplitViewMode::VERTICAL:
	//		VerticalSplitter.bisActive = true;
	//		HorizonSplitter.bisActive = false;
	//		HorizonSplitter2.bisActive = false;
	//		break;

	//	case FEditorState::SplitViewMode::HORIZONTAL:
	//		VerticalSplitter.bisActive = false;
	//		HorizonSplitter.bisActive = true;
	//		HorizonSplitter2.bisActive = false;
	//		break;

	//	case FEditorState::SplitViewMode::QUAD:
	//		VerticalSplitter.bisActive = true;
	//		HorizonSplitter.bisActive = true;
	//		HorizonSplitter2.bisActive = true;
	//		break;
	//	}
	//}
};

