#include "UObjViewerEngine.h"

#include "Source/ThirdParty/Imgui/imgui.h"
#include "Source/ThirdParty/Imgui/imgui_internal.h"
#include "Source/ThirdParty/Imgui/imgui_impl_win32.h"
#include "Source/ThirdParty/Imgui/imgui_impl_dx11.h"

#include "Source/Runtime/Rendering/ShaderConstants.h"
#include "Source/Runtime/Core/Log.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Engine/FEngineLoop.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include <commdlg.h>
#include <chrono>
#include <filesystem>

IMPLEMENT_UCLASS(UObjViewerEngine, UEngine)

void UObjViewerEngine::Init(FEngineLoop* InEngineLoop)
{
	UEngine::Init(InEngineLoop);
	HWND hWnd = EngineLoop->GetMainWindowHandle();
	ID3D11Device* Device = Renderer.GetDevice();
	ID3D11DeviceContext* Context = Renderer.GetContext();
	bDockLayoutInitialized = false;
	OnWindowSize(static_cast<UINT>(Renderer.GetWidth()), static_cast<UINT>(Renderer.GetHeight()));

	// Default Value
	Camera.SetPosition(FVector{ -5.0f, 0.0f, 0.0f });

	// ImGui Initialize
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	auto& IO = ImGui::GetIO();
	IO.Fonts->AddFontFromFileTTF(
	    "C:/Windows/Fonts/malgun.ttf",
	    18.0f,
	    nullptr,
	    IO.Fonts->GetGlyphRangesKorean());
	IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImFontConfig Config;
	Config.SizePixels = 16.0f;
	IO.Fonts->AddFontDefault(&Config);

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(Device, Context);

}

void UObjViewerEngine::Tick(float DeltaTime)
{
	FStatsManager::Get().ResetFrame();
	FInputManager::Get().BeginFrame();
	if (Globals::bIsRequestingResize)
	{
		if (Globals::ResizeWidth > 0 && Globals::ResizeHeight > 0)
		{
			Renderer.OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);
			OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);
		}
		Globals::bIsRequestingResize = false;
	}
	{
		SCOPE_CYCLE_COUNTER("Game");
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			CameraController.UpdateMouseInput(Camera);
		}
	}
	{
		SCOPE_CYCLE_COUNTER("Draw");
		Renderer.BeginFrame();
		Render();
		Renderer.SwapBuffer();
	}
	SET_CYCLE_COUNTER("Frame", FTimeManager::GetDeltaTime() * 1000.0f);
	FInputManager::Get().EndFrame();
	FInputLatencyTimer::Get().Tick();
}

void UObjViewerEngine::Render()
{
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
	ID3D11RenderTargetView* BackBufferRTV = Renderer.GetBackBufferRTV();
	// ObjViewer는 백버퍼에 바로 그리므로 백버퍼와 같은 크기의 SceneTextures를 풀에서 받아 DSV만 쓴다.
	FSceneTextures* SceneTextures = Renderer.AcquireSceneTextures(static_cast<UINT>(Renderer.GetWidth()), static_cast<UINT>(Renderer.GetHeight()));
	ID3D11DepthStencilView* SceneDepthDSV = SceneTextures ? SceneTextures->SceneDepthDSV.Get() : nullptr;

	if (BackBufferRTV && Context)
	{
		Context->OMSetRenderTargets(1, &BackBufferRTV, SceneDepthDSV);

		Context->ClearRenderTargetView(BackBufferRTV, BackgroundColor);
		if (SceneDepthDSV)
		{
			Context->ClearDepthStencilView(SceneDepthDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}

	if (CurrentMesh)
	{
		FMatrix World = FMatrix::GetIdentity();
		FViewConstants ViewConstants{
			.View = Camera.GetViewMatrix(),
			.Projection = Camera.GetProjectionMatrix(),
			.ViewportSize = FVector2{
			    static_cast<float>(Renderer.GetWidth()),
			    static_cast<float>(Renderer.GetHeight()),
			},
		};
		Renderer.UpdateViewConstants(ViewConstants);

		Renderer.UpdateLightConstants(Light);

		FObjectConstants Constants;
		Constants.World = World;
		Constants.DisableShading = 0.3f;

		// Set Material on all sections
		auto SimpleMaterial = FRenderResourceLibrary::Get().GetMaterial(FName("Simple"));
		auto TextureMaterial = FRenderResourceLibrary::Get().GetMaterial(FName("Textured"));
		for (const auto& Section : Sections)
		{
			auto It = MtlMap.find(Section.SectionName);
			if (It == MtlMap.end())
			{
				continue;
			}

			const FMtlData& Mtl = It->second;

			if (!Mtl.map_Kd.empty())
			{
				auto TexIt = TextureMap.find(Mtl.map_Kd);
				if (TexIt != TextureMap.end())
				{
					FMaterial Material = *TextureMaterial;
					Material.SetTexture(TexIt->second.get());
					Constants.Color = FVector4{ Mtl.Kd, 0.0f };
					Renderer.DrawSection(*CurrentMesh, Material, Constants, Material.GetPipeline(), Section.StartIndex, Section.IndexCount);
				}
				continue;
			}

			Constants.Color = FVector4{ Mtl.Kd, 1.0f };
			Renderer.DrawSection(*CurrentMesh, *SimpleMaterial, Constants, SimpleMaterial->GetPipeline(), Section.StartIndex, Section.IndexCount);
		}
	}

	RenderUI();
}

void UObjViewerEngine::RenderUI()
{
	// Docking
	ImGuiViewport* Viewport = ImGui::GetMainViewport();

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGuiID DockSpaceID = ImGui::DockSpaceOverViewport(0, Viewport, ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::PopStyleColor();

	// Docking Init
	if (!bDockLayoutInitialized)
	{
		ImGui::DockBuilderRemoveNode(DockSpaceID);
		ImGui::DockBuilderAddNode(DockSpaceID, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::DockBuilderSetNodeSize(DockSpaceID, Viewport->WorkSize);

		ImGuiID MainID = DockSpaceID;

		ImGuiID RightID = ImGui::DockBuilderSplitNode(MainID, ImGuiDir_Right, 0.25f, nullptr, &MainID);
		ImGuiID ConsoleID = ImGui::DockBuilderSplitNode(MainID, ImGuiDir_Down, 0.25f, nullptr, &MainID);

		ImGui::DockBuilderDockWindow("Side Bar", RightID);
		ImGui::DockBuilderDockWindow("Console Window", ConsoleID);

		ImGui::DockBuilderFinish(DockSpaceID);

		bDockLayoutInitialized = true;
	}

	// UI Render
	RenderSideBar();
	RenderConsole();
	RenderToolbar();

	// ImGui Render
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void UObjViewerEngine::Exit()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CurrentMesh.Reset();
	TextureMap.clear();
	MtlMap.clear();
	Vertices.clear();
	Indices.clear();
	Sections.clear();
	MtlDatas.clear();
	ConsoleLog.clear();
	UEngine::Exit();
}

void UObjViewerEngine::OpenObj(const char* InPath)
{
	FRawObjData RawObjData;

	// Clear Datas
	Vertices.clear();
	Indices.clear();
	Sections.clear();
	CurrentMesh = nullptr;
	CurrentObjHash = 0;

	if (FObjParser::LoadObj(InPath, RawObjData))
	{
		AddLog("Done Load Obj");
		if (FObjParser::ConvertObjToVertex(RawObjData, Vertices, Indices, Sections))
		{
			AddLog("Done Convert Obj");
			FMeshDesc MeshDesc{
				.VertexData = Vertices.data(),
				.VertexDataSize = static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
				.VertexStride = static_cast<uint32>(sizeof(FVertexData)),
				.VertexCount = static_cast<uint32>(Vertices.size()),
				.IndexData = Indices.data(),
				.IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
				.IndexCount = static_cast<uint32>(Indices.size()),
				.Sections = Sections
			};

			CurrentObjHash = FObjParser::ComputeFileHash(InPath);
			CurrentMesh = Renderer.CreateMesh(MeshDesc);
		}
	}
}

void UObjViewerEngine::ImportBinary(const char* InPath)
{
	// Clear Datas
	Vertices.clear();
	Indices.clear();
	Sections.clear();
	CurrentMesh = nullptr;

	if (FObjParser::LoadMeshFromBinary(InPath, Vertices, Indices, Sections))
	{
		FMeshDesc MeshDesc{
			.VertexData = Vertices.data(),
			.VertexDataSize = static_cast<uint32>(sizeof(FVertexData) * Vertices.size()),
			.VertexStride = static_cast<uint32>(sizeof(FVertexData)),
			.VertexCount = static_cast<uint32>(Vertices.size()),
			.IndexData = Indices.data(),
			.IndexDataSize = static_cast<uint32>(sizeof(uint32) * Indices.size()),
			.IndexCount = static_cast<uint32>(Indices.size()),
			.Sections = Sections,
		};

		CurrentMesh = Renderer.CreateMesh(MeshDesc);
	}
}

void UObjViewerEngine::ExportObjToBinary(const char* OutPath)
{
	FObjParser::SaveMeshToBinary(OutPath, CurrentObjHash, Vertices, Indices, Sections);
}

void UObjViewerEngine::OpenMtl(const char* InFilePath)
{
	MtlDatas.clear();

	bool bHasOpend = FObjParser::LoadMtl(InFilePath, MtlDatas);
	if (!bHasOpend)
	{
		AddLog("Failed to open Mtl");
		return;
	}

	// Get mtl directory path
	std::filesystem::path MtlPath = std::filesystem::path(InFilePath).parent_path();

	for (const auto& Mat : MtlDatas)
	{
		MtlMap[Mat.MaterialName] = Mat;

		// Get Texture file(diffusion only)
		if (!Mat.map_Kd.empty() && TextureMap.find(Mat.map_Kd) == TextureMap.end())
		{
			std::filesystem::path FullTexPath = MtlPath / Mat.map_Kd;

			TSharedPtr<FTexture> LoadedTexture = Renderer.CreateTexture(FullTexPath.wstring().c_str());

			if (LoadedTexture)
			{
				TextureMap[Mat.map_Kd] = LoadedTexture;
			}
		}
	}
}

void UObjViewerEngine::OnWindowSize(UINT Width, UINT Height)
{
	if (Width > 0 && Height > 0)
	{
		Camera.SetAspectRatio(static_cast<float>(Width) / static_cast<float>(Height));
	}
}

void UObjViewerEngine::RenderSideBar()
{
	ImGui::Begin("Side Bar");

	if (ImGui::CollapsingHeader("Model Statistics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Vertices  :	%zu", Vertices.size());
		ImGui::Text("Indices   :	%zu", Indices.size());
		ImGui::Text("Triangles :	%zu", Indices.size() / 3);
		ImGui::Text("Sections  :	%zu", Sections.size());
	}

	if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::ColorEdit4("Background Color", BackgroundColor);
		ImGui::Separator();
		ImGui::SliderFloat("Light Yaw", &LightYaw, -180.0f, 180.0f);
		ImGui::SliderFloat("Light Pitch", &LightPitch, -180.0f, 180.0f);
		ImGui::SliderFloat("Light Intensity", &Light.Intensity, 0.0f, 3.0f);

		float RadYaw = LightYaw * 3.141592f / 180.0f;
		float RadPitch = LightPitch * 3.141592f / 180.0f;
		FVector NewDirection = FVector(cosf(RadPitch) * cosf(RadYaw), cosf(RadPitch) * sinf(RadYaw), sinf(RadPitch));
		NewDirection.Normalize();
		Light.LightDirection = NewDirection;
	}

	ImGui::End();
}

void UObjViewerEngine::RenderConsole()
{
	ImGui::Begin("Console Window");

	if (ImGui::Button("Clear Logs"))
	{
		ConsoleLog.clear();
	}

	ImGui::Separator();

	ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	for (const auto& Log : ConsoleLog)
	{
		ImGui::TextUnformatted(Log.c_str());
	}

	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();

	ImGui::End();
}

void UObjViewerEngine::RenderToolbar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::Button("Open(Obj)"))
		{
			FString Path;
			constexpr wchar_t Filter[] = L"Wavefront OBJ (*.obj)\0*.obj\0All Files (*.*)\0*.*\0";
			if (PickFile(Path, Filter, L"obj", false))
			{
				auto Start = std::chrono::high_resolution_clock::now();

				OpenObj(Path.c_str());
				FString MtlPath = Path.substr(0, Path.find_last_of('.')) + ".mtl";
				OpenMtl(MtlPath.c_str());

				auto End = std::chrono::high_resolution_clock::now();
				float Elapsed = std::chrono::duration<float, std::milli>(End - Start).count();

				AddLog("[Open Obj]" + Path + " " + std::to_string(Elapsed) + "ms");
			}
		}
		if (ImGui::Button("Import(Bin)"))
		{
			FString Path;
			constexpr wchar_t Filter[] = L"Binary BIN (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
			if (PickFile(Path, Filter, L"bin", false))
			{
				auto Start = std::chrono::high_resolution_clock::now();

				ImportBinary(Path.c_str());
				FString MtlPath = Path.substr(0, Path.find_last_of('.')) + ".mtl";
				OpenMtl(MtlPath.c_str());

				auto End = std::chrono::high_resolution_clock::now();
				float Elapsed = std::chrono::duration<float, std::milli>(End - Start).count();
				AddLog("[Import Bin]" + Path + " " + std::to_string(Elapsed) + "ms");
			}
		}
		if (ImGui::Button("Export(Bin)"))
		{
			FString Path;
			constexpr wchar_t Filter[] = L"Binary BIN (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
			if (PickFile(Path, Filter, L"bin", true))
			{
				auto Start = std::chrono::high_resolution_clock::now();

				ExportObjToBinary(Path.c_str());

				auto End = std::chrono::high_resolution_clock::now();
				float Elapsed = std::chrono::duration<float, std::milli>(End - Start).count();
				AddLog("[Export Bin]" + Path + " " + std::to_string(Elapsed) + "ms");
			}
		}

		ImGui::EndMainMenuBar();
	}
}

void UObjViewerEngine::AddLog(const FString& Message)
{
	ConsoleLog.push_back(Message);
}

bool UObjViewerEngine::PickFile(FString& OutPath, const wchar_t* InFileFilter, LPCWSTR InlpstrDefExt, bool bSave)
{
	wchar_t Buffer[MAX_PATH]{};

	OPENFILENAMEW Desc{};
	Desc.lStructSize = sizeof(Desc);
	Desc.hwndOwner = static_cast<HWND>(ImGui::GetMainViewport()->PlatformHandleRaw);
	Desc.lpstrFilter = InFileFilter;
	Desc.lpstrFile = Buffer;
	Desc.nMaxFile = MAX_PATH;
	Desc.lpstrDefExt = InlpstrDefExt;

	Desc.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | (bSave ? OFN_OVERWRITEPROMPT : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST));

	if (!(bSave ? GetSaveFileNameW(&Desc) : GetOpenFileNameW(&Desc)))
	{
		return false;
	}

	OutPath = std::filesystem::path(Buffer).string();
	return true;
}
