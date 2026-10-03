#pragma once

#include "FEditorViewportClient.h"
#include "Runtime/Engine/FViewport.h"

constexpr uint32 MAX_VIEWPORT_COUNT = 4;

struct FEditorViewport
{
	FEditorViewportClient Client = {};
	FViewport Viewport = {};

	FEditorViewport()
	{
		Client.SetViewPort(&Viewport);
	}

	FEditorViewport(const FEditorViewport&) = delete;
	FEditorViewport& operator=(const FEditorViewport&) = delete;
};
