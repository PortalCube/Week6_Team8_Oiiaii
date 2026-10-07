#include "FImguiToolBar.h"
#include "ThirdParty/Imgui/imgui.h"
#include "Editor/Core/FEditor.h"
#include "FImguiEditorViewportWindow.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/Utility/WindowsUtil.h"
// "표시명\0패턴\0" 이중 널 종료 필요
constexpr wchar_t SceneFilter[] = L"Scene Files (*.Scene)\0*.Scene\0All Files (*.*)\0*.*\0";
constexpr wchar_t ObjFilter[] = L"Scene Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";

void FImguiToolbar::Process(FEditor& Editor, float DeltaTime)
{
	if (Editor.bZenMode)
	{
		return;
	}

	static FString CurrentScenePath;

	if (ImGui::BeginMainMenuBar())
	{
		// 씬 저장,로드 기능
		ShowFileBar(CurrentScenePath, Editor);

		// Imgui Window들 소환
		ShowViewBar(Editor);

		ShowPIEBar(Editor);

		ImGui::EndMainMenuBar();
	}
}

// 취소하면 false
bool FImguiToolbar::PickSceneFile(FString& OutPath, bool bSave)
{
	wchar_t Buffer[MAX_PATH]{};

	OPENFILENAMEW Desc{};
	Desc.lStructSize = sizeof(Desc);
	Desc.hwndOwner = static_cast<HWND>(ImGui::GetMainViewport()->PlatformHandleRaw);
	Desc.lpstrFilter = SceneFilter;
	Desc.lpstrFile = Buffer;
	Desc.nMaxFile = MAX_PATH;
	Desc.lpstrDefExt = L"Scene";
	// OFN_NOCHANGEDIR 없으면 대화상자가 프로세스 현재 디렉터리를 바꿔서
	// 이후 상대 경로 로딩(셰이더/텍스처)이 조용히 깨진다
	Desc.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | (bSave ? OFN_OVERWRITEPROMPT : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST));

	if (!(bSave ? GetSaveFileNameW(&Desc) : GetOpenFileNameW(&Desc)))
		return false;

	// 씬 경로는 FileUtil(UTF-8 경로 규약)로 읽고 쓰므로 UTF-8로 변환
	OutPath = WindowsUtil::ToString(Buffer);
	return true;
}

void FImguiToolbar::ShowFileBar(FString CurrentScenePath, FEditor& Editor)
{

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Scene"))
		{
			Editor.NewScene();
			CurrentScenePath.clear();
		}
		if (ImGui::MenuItem("Save Scene"))
		{
			// 경로가 있으면 그대로 덮어쓰고, 없으면 다른 이름으로 저장과 같게 동작
			if (CurrentScenePath.empty())
			{
				FString Path;
				if (PickSceneFile(Path, true))
				{
					CurrentScenePath = Path;
					Editor.SaveScene(Path);
				}
			}
			else
			{
				Editor.SaveScene(CurrentScenePath);
			}
		}

		if (ImGui::MenuItem("Save scene as..."))
		{
			FString Path;
			if (PickSceneFile(Path, true))
			{
				CurrentScenePath = Path;
				Editor.SaveScene(Path);
			}
		}

		if (ImGui::MenuItem("Load Scene"))
		{
			FString Path;
			if (PickSceneFile(Path, false))
			{
				CurrentScenePath = Path;
				Editor.LoadScene(Path);
			}
		}

		if (ImGui::MenuItem("Import Import"))
		{
			FString Path;
			if (PickObjFile(Path))
			{
				FResourceLoader::ImportObj(Path);
			}
		}

		ImGui::EndMenu();
	}
}

void FImguiToolbar::ShowViewBar(FEditor& Editor)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::BeginMenu("ViewPort"))
		{
			if (ImGui::MenuItem("Single"))
			{
				Editor.SetViewLayout(FEditorState::SplitViewMode::SINGLE);
			}
			if (ImGui::MenuItem("Top | Bottom"))
			{
				Editor.SetViewLayout(FEditorState::SplitViewMode::VERTICAL);
			}
			if (ImGui::MenuItem("Left | Right"))
			{
				Editor.SetViewLayout(FEditorState::SplitViewMode::HORIZONTAL);
			}
			if (ImGui::MenuItem("2 X 2"))
			{
				Editor.SetViewLayout(FEditorState::SplitViewMode::QUAD);
			}

			ImGui::EndMenu();
		}

		ImGui::MenuItem("Hide UI", nullptr, &Editor.bHideUI);
		ImGui::MenuItem("Show Benchmark UI", nullptr, &Editor.bShowBenchmark);

		ImGui::EndMenu();
	}

	auto& Gizmo = Editor.GetGizmo();

	static const char* GizmoModes[4] = { "None", "Translation", "Rotation", "Scale" };
	const int SelectedItem = static_cast<int>(Editor.GetGizmo().Mode);
	if (ImGui::Button(GizmoModes[SelectedItem], { 150.0f, 0.0f }))
	{
		Gizmo.Mode = static_cast<EGizmoMode>((SelectedItem + 1) % 4);
	}
}

void FImguiToolbar::ShowPIEBar(FEditor& Editor)
{
	const EPIESessionState ButtonState = Editor.GetPIEState();
	const bool bTransitioning = ButtonState == EPIESessionState::Starting || ButtonState == EPIESessionState::Stopping;

	if (!Editor.GetViewportLayout().Root)
		return;

	// 전체 뷰포트 영역의 가로 중앙
	const FRect& Rect = Editor.GetViewportLayout().Root->Rect;
	const float CenterX = ImGui::GetMainViewport()->Pos.x + (Rect.Left + Rect.Right) * 0.5f;

	const ImVec2 ButtonSize{ 40.0f, ImGui::GetFrameHeight() };

	// const float ButtonSpacing = ImGui::GetStyle().ItemSpacing.x;
	const float ButtonSpacing = ButtonSize.x * 0.025f;
	const float TotalWidth = ButtonSize.x * 2.0f + ButtonSpacing;
	ImVec2 Cursor = ImGui::GetCursorScreenPos();
	Cursor.x = CenterX - TotalWidth * 0.5f;
	ImGui::SetCursorScreenPos(Cursor);

	// 재생 / 일시정지
	ImGui::BeginDisabled(bTransitioning);
	if (ImGui::Button("##PIE", ButtonSize))
	{
		switch (ButtonState)
		{
		case EPIESessionState::Stopped:
		{
			FRequestPlaySessionParams Params{};
			Editor.RequestStartPIE(Params);
			break;
		}

		case EPIESessionState::Running:
			Editor.PausePIE();
			break;

		case EPIESessionState::Paused:
			Editor.ResumePIE();
			break;

		default:
			break;
		}
	}

	ImGui::EndDisabled();

	const bool bIsPIERunning = Editor.GetPIEState() == EPIESessionState::Running;

	const ImVec2 Min = ImGui::GetItemRectMin();
	const ImVec2 Max = ImGui::GetItemRectMax();

	const float X = (Min.x + Max.x) * 0.5f;
	const float Y = (Min.y + Max.y) * 0.5f;
	const float H = (Max.y - Min.y) * 0.3f;

	ImDrawList* DrawList = ImGui::GetWindowDrawList();
	const ImU32 Color = IM_COL32(160, 205, 90, 255);

	if (bIsPIERunning)
	{
		// 일시정지 아이콘 Ⅱ
		DrawList->AddRectFilled(
		    ImVec2{ X - H * 0.7f, Y - H },
		    ImVec2{ X - H * 0.2f, Y + H },
		    Color);

		DrawList->AddRectFilled(
		    ImVec2{ X + H * 0.2f, Y - H },
		    ImVec2{ X + H * 0.7f, Y + H },
		    Color);
	}
	else
	{
		// 재생 아이콘 ▶
		DrawList->AddTriangleFilled(
		    ImVec2{ X - H * 0.5f, Y - H },
		    ImVec2{ X + H, Y },
		    ImVec2{ X - H * 0.5f, Y + H },
		    Color);
	}

	ImGui::SameLine(0.0f, ButtonSpacing);

	const EPIESessionState StopState = Editor.GetPIEState();
	const bool bCanStop =
		StopState == EPIESessionState::Starting ||
	    StopState == EPIESessionState::Running ||
	    StopState == EPIESessionState::Paused;

	ImGui::BeginDisabled(!bCanStop);

	if (ImGui::Button("##StopPIE", ButtonSize))
	{
		Editor.RequestEndPIE();
	}

	ImGui::EndDisabled();

	const ImVec2 StopMin = ImGui::GetItemRectMin();
	const ImVec2 StopMax = ImGui::GetItemRectMax();
	const float StopX = (StopMin.x + StopMax.x) * 0.5f;
	const float StopY = (StopMin.y + StopMax.y) * 0.5f;
	const float HalfSize = (StopMax.y - StopMin.y) * 0.3f;

	// 중지 아이콘 ■
	const bool bHasSession = Editor.GetPIEState() != EPIESessionState::Stopped;
	const ImU32 StopColor = bHasSession ? IM_COL32(220, 70, 70, 255) : IM_COL32(180, 180, 180, 255); // 실행 중: 빨간색 / 기본: 회색 

	DrawList->AddRectFilled(
	    ImVec2{ StopX - HalfSize, StopY - HalfSize },
	    ImVec2{ StopX + HalfSize, StopY + HalfSize },
	    StopColor);
}

bool FImguiToolbar::PickObjFile(FString& OutPath)
{
	wchar_t Buffer[MAX_PATH]{};

	OPENFILENAMEW Desc{};
	Desc.lStructSize = sizeof(Desc);
	Desc.hwndOwner = static_cast<HWND>(ImGui::GetMainViewport()->PlatformHandleRaw);
	Desc.lpstrFilter = ObjFilter;
	Desc.lpstrFile = Buffer;
	Desc.nMaxFile = MAX_PATH;
	Desc.lpstrDefExt = L"obj";
	Desc.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (!GetOpenFileNameW(&Desc))
	{
		return false;
	}

	OutPath = std::filesystem::path(Buffer).string();
	return true;
}
