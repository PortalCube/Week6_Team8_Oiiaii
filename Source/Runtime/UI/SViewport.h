#pragma once

#include "SWindow.h"

// 화면에 실제로 표시되는 leaf 뷰포트
class SViewport final : public SWindow
{
public:
	SViewport() = default;
	SViewport(FEditorViewport* InEditorViewport)
	    : EditorViewport(InEditorViewport) {}

	SViewport(const SViewport&) = delete;
	SViewport& operator=(const SViewport&) = delete;
	bool operator==(const SViewport& Other) const { return this->EditorViewport == Other.EditorViewport; }


public:
	FEditorViewport* EditorViewport = nullptr;

	void ResizeViewport(const FRect& Rect, const FVector2& ClientSize)
	{
		EditorViewport->Client.ResizeViewport(Rect);
	}
};
