#pragma once
#include "Editor/UI/IEditorWindow.h"
#include "Runtime/Core/FString.h"

class FImguiToolbar final : public IEditorWindow
{

public:
	FImguiToolbar() = default;
	~FImguiToolbar() = default;

	// 복사 생성 금지
	FImguiToolbar(const FImguiToolbar&) = delete;
	// 복사 대입 금지
	FImguiToolbar& operator=(const FImguiToolbar&) = delete;

	void Process(FEditor& Editor, float DeltaTime) override;

	FString ToNarrow(const wchar_t* Wide);
	bool PickSceneFile(FString& OutPath, bool bSave);

	void ShowFileBar(FString CurrentScenePath, FEditor& Editor);
	void ShowViewBar(FEditor& Editor);
	void ShowPIEBar(FEditor& Editor);

	bool PickObjFile(FString& OutPath);
};
