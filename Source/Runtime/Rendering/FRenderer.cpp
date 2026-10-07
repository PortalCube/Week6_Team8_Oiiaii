#include "Runtime/Rendering/FBillboardRendering.h"
#include "FRenderer.h"
#include "FTextRendering.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Asset/UFont.h"
#include "Runtime/Asset/UMaterial.h"
#include "Runtime/Asset/UStaticMesh.h"
#include "FRenderResourceLibrary.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <Windows.h>
#include <wrl/client.h>

#include "FMaterial.h"
#include "FMesh.h"
#include "FRenderPipeline.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Core/Log.h"
#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/CoreUObject/FStatsManager.h"
#include "Runtime/Components/UFireBallComponent.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Engine/FSceneView.h"
#include "Runtime/Rendering/FRenderQueue.h"
#include "Runtime/Rendering/FTexture.h"
#include "ShaderConstants.h"
#include "Vertices.h"

#include "ThirdParty/DirectXTK/Inc/DDSTextureLoader.h"
#include "ThirdParty/DirectXTK/Inc/WICTextureLoader.h"

bool FRenderer::Initialize(HWND Window)
{
	if (!InitializeDeviceAndSwapChain(Window) ||
	    !InitializeBackBuffer() ||
		!InitializeConstantBuffers() ||
	    !InitializePointLightBuffers())
	{
		Shutdown();
		return false;
	}

	LineBatcher.Initialize(Device.Get()); // batch line

	// 실패해도 렌더링은 되므로 GPU Time 스탯만 0으로 남는다.
	InitializeGPUTimerQueries();

	return true;
}

void FRenderer::Shutdown()
{
	if (Context)
	{
		Context->ClearState();
		Context->Flush();
	}

	LineBatcher.Shutdown();

	RasterizerStateMap.clear();
	DepthStencilStateMap.clear();
	BlendStateMap.clear();
	SamplerStateMap.clear();

	ObjectConstantUploadBuffer.Reset();
	// ObjectConstantBuffer.Reset();
	// FrameConstantBuffer.Reset();
	// ViewConstantBuffer.Reset();
	// ObjectConstantBuffer.Reset();
	// ViewConstantBuffer.Reset();
	for (int32 i = 0; i < NumFrameResourceCount; ++i)
	{
		FrameResources[i].FrameConstantBuffer.Reset();
		FrameResources[i].ObjectConstantBuffer.Reset();
		FrameResources[i].ViewConstantBuffer.Reset();
		FrameResources[i].FXAAConstantBuffer.Reset();
	}
	LightConstantBuffer.Reset();
	PointLightSRV.Reset();
	PointLightBuffer.Reset();
	PointLightCountBuffer.Reset();


	BackBufferTexture.Reset();
	BackBufferRTV.Reset();
	SceneTexturesPool.clear();
	ActiveSceneTextures = nullptr;
	CachedBillboardText.clear();
	CachedBillboardFont = nullptr;
	CachedBillboardGlyphs.clear();
	BillboardWorldInstances.clear();

	for (FGPUTimerQuery& Query : GPUTimerQueries)
	{
		Query.Disjoint.Reset();
		Query.Start.Reset();
		Query.End.Reset();
		Query.bPending = false;
		Query.InputStartTick = 0;
	}

	SwapChain.Reset();
	Context1.Reset();
	Context.Reset();
	Device.Reset();
}

void FRenderer::BeginFrame()
{
	CurrentFrameResourceIndex = (CurrentFrameResourceIndex + 1) % NumFrameResourceCount;
	BeginGPUTimer();

	ActiveSceneTextures = nullptr;
	// 안 쓰는 SceneTexture 정리
	++FrameCounter;
	EvictUnusedSceneTextures();

	Context->RSSetViewports(1, &Viewport);

	// 백버퍼만 바인딩, Clear
	BindRenderTarget(BackBufferRTV.Get(), nullptr);
	Context->ClearRenderTargetView(BackBufferRTV.Get(), ClearColor);
}

// RTV와 DSV를 컨텍스트에 바인딩
void FRenderer::BindRenderTarget(ID3D11RenderTargetView* RTV, ID3D11DepthStencilView* DSV)
{
	ID3D11RenderTargetView* RTVs[] = { RTV };
	Context->OMSetRenderTargets(1, RTVs, DSV);
}

// 뷰포트가 그릴 픽셀 영역을 컨텍스트에 바인딩
void FRenderer::SetViewportPixel(FVector2 ViewportSizePixel)
{
	D3D11_VIEWPORT RenderViewport = Viewport;
	RenderViewport.TopLeftX = 0.f;
	RenderViewport.TopLeftY = 0.f;
	RenderViewport.Width = ViewportSizePixel.X;
	RenderViewport.Height = ViewportSizePixel.Y;
	Context->RSSetViewports(1, &RenderViewport);
};

// 현재 뷰포트의 SceneTextures 깊이/스텐실을 비운다
void FRenderer::ClearDepth()
{
	if (!ActiveSceneTextures)
	{
		return;
	}
	Context->ClearDepthStencilView(ActiveSceneTextures->SceneDepthDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

// 크기에 맞는 뷰포트를 만들어 반환한다
D3D11_VIEWPORT FRenderer::MakeD3DViewport(float Left, float Top, float Width, float Height) const
{
	D3D11_VIEWPORT Result = Viewport; // MinDepth / MaxDepth 유지
	Result.TopLeftX = Left;
	Result.TopLeftY = Top;
	Result.Width = Width;
	Result.Height = Height;
	return Result;
}

// 뷰포트의 출력 렌더 타깃을 준비
bool FRenderer::PrepareViewportRenderTarget(FViewport& InViewport)
{
	if (!Device)
	{
		return false;
	}

	// 크기는 뷰포트 Rect에서 읽는다
	const FVector2 Size = InViewport.GetViewportSize();
	if (Size.X <= 0.0f || Size.Y <= 0.0f)
	{
		return false;
	}
	const UINT Width = static_cast<UINT>(Size.X);
	const UINT Height = static_cast<UINT>(Size.Y);

	const FViewportRenderTarget* Current = InViewport.RenderTarget.get();
	
	// 없거나, 리사이즈 플래그가 켜졌거나, 크기가 다르면 새로 만든다.
	const bool bNeedCreate = !Current || InViewport.IsResizeRenderTarget() || Current->GetWidth() != Width || Current->GetHeight() != Height;

	if (bNeedCreate)
	{
		// 뷰포트의 FViewportRenderTarget 생성: Texture, SRV, RTV 생성
		TSharedPtr<FViewportRenderTarget> NewRenderTarget{ new FViewportRenderTarget() };
		if (!NewRenderTarget || !NewRenderTarget->Initialize(Device.Get(), Width, Height))
		{
			// 실패: 이번 프레임은 이 뷰포트를 건너뛰고, 플래그를 유지해서 다음 프레임에 다시 시도한다
			InViewport.SetResizeRenderTarget(true);
			return false;
		}

		InViewport.RenderTarget = std::move(NewRenderTarget);
		InViewport.SetResizeRenderTarget(false);
	}

	return true;
}

// Width, Height 크기의 SceneTextures를 찾거나 만들어서 ActiveSceneTextures로 설정
FSceneTextures* FRenderer::AcquireSceneTextures(UINT Width, UINT Height)
{
	ActiveSceneTextures = nullptr;

	if (!Device || Width == 0 || Height == 0)
	{
		return nullptr;
	}

	const std::pair<UINT, UINT> Key{ Width, Height };
	auto It = SceneTexturesPool.find(Key);
	if (It == SceneTexturesPool.end())
	{
		// 이 크기는 처음 -> 정확히 이 크기로 만든다
		FSceneTextures NewTextures;
		if (!NewTextures.InitializeSceneTextures(Device.Get(), Width, Height))
		{
			return nullptr;
		}
		It = SceneTexturesPool.emplace(Key, std::move(NewTextures)).first;
	}

	It->second.LastUsedFrame = FrameCounter;
	ActiveSceneTextures = &It->second;
	return ActiveSceneTextures;
}

void FRenderer::EvictUnusedSceneTextures()
{
	// 스플리터를 드래그하면 매 프레임 새로운 크기가 생기므로, 최근에 안 쓰인 크기는 지움
	for (auto It = SceneTexturesPool.begin(); It != SceneTexturesPool.end();)
	{
		if (FrameCounter - It->second.LastUsedFrame > SceneTexturesKeepFrames)
		{
			It = SceneTexturesPool.erase(It);
		}
		else
		{
			++It;
		}
	}
}

//백버퍼의 InViewport.Rect 위치에 뷰포트 출력 렌더 타깃을 합성
void FRenderer::CompositeViewport(const FViewport& InViewport)
{
	const FViewportRenderTarget* RenderTarget = InViewport.RenderTarget.get();
	if (!RenderTarget || !RenderTarget->GetSRV())
	{
		return;
	}

	// 백버퍼 안에서 이 뷰포트가 그려질 위치 (메인 창 클라이언트 영역 기준 픽셀 좌표)
	const FRect& Rect = InViewport.Rect;
	const D3D11_VIEWPORT TargetD3DViewport = MakeD3DViewport(Rect.Left, Rect.Top, Rect.GetWidth(), Rect.GetHeight());

	ID3D11ShaderResourceView* SRVs[] = { RenderTarget->GetSRV() };
	DrawScreenPass(BackBufferRTV.Get(), TargetD3DViewport, SRVs, 1, FName("#Composite"));
}

void FRenderer::FlushDrawStats()
{
	if (PendingDrawCount != 0u)
	{
		INC_DWORD_STAT_BY("Draws", PendingDrawCount);
		INC_DWORD_STAT_BY("Prims", PendingPrimCount);
	}
	PendingDrawCount = 0u;
	PendingPrimCount = 0u;
}

void FRenderer::SwapBuffer()
{
	FlushDrawStats();
	EndGPUTimer();
	ResolveGPUTimer();

	SwapChain->Present(0u, 0u);
}

void FRenderer::OnWindowSize(UINT Width, UINT Height)
{
	Context->OMSetRenderTargets(0, nullptr, nullptr);
	BackBufferTexture.Reset();
	BackBufferRTV.Reset();
	// 창 크기가 바뀌면 기존 크기의 SceneTextures는 더 이상 쓰이지 않으므로 풀을 비운다.
	ActiveSceneTextures = nullptr;
	SceneTexturesPool.clear();


	SwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0);
	Viewport.Width = static_cast<float>(Width);
	Viewport.Height = static_cast<float>(Height);

	InitializeBackBuffer();
}

TSharedPtr<FMesh> FRenderer::CreateMesh(const FMeshDesc& Desc)
{
	if (!Desc.VertexData || Desc.VertexCount == 0 || Desc.VertexDataSize == 0 || Desc.VertexStride == 0)
	{
		return nullptr;
	}
	if (Desc.IndexCount > 0 && (!Desc.IndexData || Desc.IndexDataSize == 0))
	{
		return nullptr;
	}

	auto Mesh = TSharedPtr<FMesh>{ new FMesh() };
	D3D11_BUFFER_DESC VertexBufferDesc = {
		.ByteWidth = Desc.VertexDataSize,
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
	};

	D3D11_SUBRESOURCE_DATA VertexData = {
		.pSysMem = Desc.VertexData,
	};

	HRESULT Result =
	    Device->CreateBuffer(&VertexBufferDesc, &VertexData, &Mesh->VertexBuffer);
	if (FAILED(Result))
	{
		return nullptr;
	}
	Mesh->VertexBufferSize = Desc.VertexDataSize;
	Mesh->VertexCount = Desc.VertexCount;
	Mesh->VertexStride = Desc.VertexStride;

	if (Desc.IndexCount > 0 && Desc.IndexData)
	{
		D3D11_BUFFER_DESC IndexBufferDesc = {
			.ByteWidth = Desc.IndexDataSize,
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
		};

		D3D11_SUBRESOURCE_DATA IndexData = {
			.pSysMem = Desc.IndexData,
		};

		Result = Device->CreateBuffer(&IndexBufferDesc, &IndexData, &Mesh->IndexBuffer);
		if (FAILED(Result))
		{
			return nullptr;
		}
	}
	Mesh->IndexBufferSize = Desc.IndexDataSize;
	Mesh->IndexCount = Desc.IndexCount;

	const auto* vertices = static_cast<const FVertexData*>(Desc.VertexData);

	Mesh->Positions.reserve(Desc.VertexCount);
	for (uint32 i = 0; i < Desc.VertexCount; ++i)
	{
		Mesh->Positions.push_back(FVector{ vertices[i].x, vertices[i].y, vertices[i].z });
	}

	if (Desc.IndexCount > 0)
	{
		const auto* indices = static_cast<const uint32*>(Desc.IndexData);
		Mesh->Indices.assign(indices, indices + Desc.IndexCount);
	}

	if (Desc.bBuildBVH)
	{
		Mesh->BuildTriangleVertices();
	}

	Mesh->Topology = Desc.bIsLine ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST
	                              : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	Mesh->LocalBounds = FAxisAlignedBoundingBox{ *Mesh.get() };

	Mesh->Sections = Desc.Sections;

	return Mesh;
}

// 텍스트 렌더링용
TSharedPtr<FMesh> FRenderer::CreateDynamicMesh(const FMeshDesc& Desc)
{
	if (!Desc.VertexData || Desc.VertexCount == 0 || Desc.VertexDataSize == 0 || Desc.VertexStride == 0)
	{
		return nullptr;
	}
	if (Desc.IndexCount > 0 && (!Desc.IndexData || Desc.IndexDataSize == 0))
	{
		return nullptr;
	}

	auto Mesh = TSharedPtr<FMesh>{ new FMesh() };
	D3D11_BUFFER_DESC VertexBufferDesc = {
		.ByteWidth = Desc.VertexDataSize,
		.Usage = D3D11_USAGE_DYNAMIC,
		.BindFlags = D3D11_BIND_VERTEX_BUFFER,
		.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
	};

	D3D11_SUBRESOURCE_DATA VertexData = {
		.pSysMem = Desc.VertexData,
	};

	HRESULT Result = Device->CreateBuffer(&VertexBufferDesc, &VertexData, &Mesh->VertexBuffer);
	if (FAILED(Result))
	{
		return nullptr;
	}

	Mesh->VertexCount = Desc.VertexCount;
	Mesh->VertexStride = Desc.VertexStride;
	Mesh->VertexBufferSize = Desc.VertexDataSize;

	if (Desc.IndexCount > 0 && Desc.IndexData)
	{
		D3D11_BUFFER_DESC IndexBufferDesc = {
			.ByteWidth = Desc.IndexDataSize,
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
		};

		D3D11_SUBRESOURCE_DATA IndexData = {
			.pSysMem = Desc.IndexData,
		};

		Result = Device->CreateBuffer(&IndexBufferDesc, &IndexData, &Mesh->IndexBuffer);
		if (FAILED(Result))
		{
			return nullptr;
		}
	}
	Mesh->IndexCount = Desc.IndexCount;
	Mesh->IndexBufferSize = Desc.IndexDataSize;

	const auto* vertices = static_cast<const FVertexData*>(Desc.VertexData);

	Mesh->Positions.reserve(Desc.VertexCount);
	for (uint32 i = 0; i < Desc.VertexCount; ++i)
	{
		Mesh->Positions.push_back(FVector{ vertices[i].x, vertices[i].y, vertices[i].z });
	}

	if (Desc.IndexCount > 0)
	{
		const auto* indices = static_cast<const uint32*>(Desc.IndexData);
		Mesh->Indices.assign(indices, indices + Desc.IndexCount);
	}

	Mesh->Topology = Desc.bIsLine ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST
	                              : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	Mesh->LocalBounds = FAxisAlignedBoundingBox{ *Mesh.get() };
	return Mesh;
}

void FRenderer::GetDeviceAndContext_ImplDX11(ID3D11Device*& DeviceOut,
    ID3D11DeviceContext*& ContextOut)
{
	DeviceOut = Device.Get();
	ContextOut = Context.Get();
}

TSharedPtr<FRenderPipeline>
FRenderer::CreateRenderPipeline(const FRenderPipelineDesc& Desc)
{
	namespace fs = std::filesystem;

	Microsoft::WRL::ComPtr<ID3DBlob> Blob;
	const fs::path VertexShaderPath{ Desc.VertexShaderFilePath };
	HRESULT Result = D3DReadFileToBlob(VertexShaderPath.wstring().c_str(), &Blob);
	if (FAILED(Result))
	{
		return nullptr;
	}

	Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader;
	Result = Device->CreateVertexShader(Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &VertexShader);
	if (FAILED(Result))
	{
		return nullptr;
	}

	size_t VSSize = Blob->GetBufferSize();
	INC_MEMORY_STAT_BY("VertexShaderMemory", Blob->GetBufferSize());

	Microsoft::WRL::ComPtr<ID3D11InputLayout> InputLayout;
	if (Desc.bIsInstancing)
	{
		Result = Device->CreateInputLayout(FVertexInstanceLayouts::Layout, FVertexInstanceLayouts::NumElements, Blob->GetBufferPointer(), Blob->GetBufferSize(), &InputLayout);
	}
	else
	{
		Result = Device->CreateInputLayout(FVertexLayouts::Layout, FVertexLayouts::NumElements, Blob->GetBufferPointer(), Blob->GetBufferSize(), &InputLayout);
	}

	if (FAILED(Result))
	{
		return nullptr;
	}

	const fs::path PixelShaderPath{ Desc.PixelShaderFilePath };
	Result = D3DReadFileToBlob(PixelShaderPath.wstring().c_str(), &Blob);
	if (FAILED(Result))
	{
		return nullptr;
	}

	Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader;
	Result = Device->CreatePixelShader(Blob->GetBufferPointer(), Blob->GetBufferSize(), nullptr, &PixelShader);
	if (FAILED(Result))
	{
		return nullptr;
	}

	size_t PSSize = Blob->GetBufferSize();
	INC_MEMORY_STAT_BY("PixelShaderMemory", Blob->GetBufferSize());

	auto RasterizerState = GetOrCreateRasterizerState(Desc.Rasterizer);
	auto DepthStencilState = GetOrCreateDepthStencilState(Desc.DepthStencil);
	auto BlendState = GetOrCreateBlendState(Desc.Blend);
	auto SamplerState = GetOrCreateSamplerState(FTextureSamplerDesc{});

	if (!RasterizerState || !DepthStencilState || !BlendState || !SamplerState)
	{
		return nullptr;
	}

	FRenderPipelineCreateInfo CreateInfo{
		.Desc = std::move(Desc),
		.VertexShader = std::move(VertexShader),
		.PixelShader = std::move(PixelShader),
		.InputLayout = std::move(InputLayout),
		.RasterizerState = std::move(RasterizerState),
		.DepthStencilState = std::move(DepthStencilState),
		.SamplerState = std::move(SamplerState),
		.BlendState = std::move(BlendState),
	};

	TSharedPtr<FRenderPipeline> Pipeline = MakeShared<FRenderPipeline>(std::move(CreateInfo));

	Pipeline->SetVertexShaderSize(VSSize);
	Pipeline->SetPixelShaderSize(PSSize);

	return Pipeline;
}

Microsoft::WRL::ComPtr<ID3D11RasterizerState>
FRenderer::GetOrCreateRasterizerState(const FRasterizerDesc& Desc)
{
	if (const auto It = RasterizerStateMap.find(Desc); It != RasterizerStateMap.end())
	{
		return It->second;
	}

	static const TMap<ERasterizerFillMode, D3D11_FILL_MODE> FillModeMap{
		{ ERasterizerFillMode::Solid, D3D11_FILL_SOLID },
		{ ERasterizerFillMode::Wireframe, D3D11_FILL_WIREFRAME },
	};

	static const TMap<ERasterizerCullMode, D3D11_CULL_MODE> CullModeMap{
		{ ERasterizerCullMode::None, D3D11_CULL_NONE },
		{ ERasterizerCullMode::Front, D3D11_CULL_FRONT },
		{ ERasterizerCullMode::Back, D3D11_CULL_BACK },
	};
	static const TMap<ERasterizerFrontFaceMode, BOOL> FrontFaceMap{
		{ ERasterizerFrontFaceMode::CounterClockwise, TRUE },
		{ ERasterizerFrontFaceMode::Clockwise, FALSE },
	};

	const D3D11_RASTERIZER_DESC NativeDesc{
		.FillMode = FillModeMap.at(Desc.FillMode),
		.CullMode = CullModeMap.at(Desc.CullMode),
		.FrontCounterClockwise = FrontFaceMap.at(Desc.FrontFace),
		.MultisampleEnable = Desc.bUseMultisample,
		.AntialiasedLineEnable = Desc.bUseAntialiasedLine,
	};

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> State;
	if (FAILED(Device->CreateRasterizerState(&NativeDesc, &State)))
	{
		return nullptr;
	}
	RasterizerStateMap.emplace(Desc, State);
	return State;
}

Microsoft::WRL::ComPtr<ID3D11DepthStencilState>
FRenderer::GetOrCreateDepthStencilState(const FDepthStencilDesc& Desc)
{
	if (const auto It = DepthStencilStateMap.find(Desc); It != DepthStencilStateMap.end())
	{
		return It->second;
	}

	static const TMap<EDepthWriteMode, D3D11_DEPTH_WRITE_MASK> DepthWriteMap{
		{ EDepthWriteMode::Disable, D3D11_DEPTH_WRITE_MASK_ZERO },
		{ EDepthWriteMode::Enable, D3D11_DEPTH_WRITE_MASK_ALL },
	};

	D3D11_DEPTH_STENCIL_DESC NativeDesc{};
	NativeDesc.DepthEnable = Desc.bDepthEnable;
	NativeDesc.DepthWriteMask = DepthWriteMap.at(Desc.DepthWrite);
	NativeDesc.DepthFunc = D3D11_COMPARISON_LESS;
	NativeDesc.StencilEnable = Desc.bStencilEnable;
	NativeDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	NativeDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	NativeDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	NativeDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	NativeDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	NativeDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	NativeDesc.BackFace = NativeDesc.FrontFace;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> State;
	if (FAILED(Device->CreateDepthStencilState(&NativeDesc, &State)))
	{
		return nullptr;
	}
	DepthStencilStateMap.emplace(Desc, State);
	return State;
}

Microsoft::WRL::ComPtr<ID3D11BlendState>
FRenderer::GetOrCreateBlendState(const FBlendDesc& Desc)
{
	if (const auto It = BlendStateMap.find(Desc); It != BlendStateMap.end())
	{
		return It->second;
	}

	static const TMap<EBlendMode, D3D11_RENDER_TARGET_BLEND_DESC> BlendModeMap{
		{ EBlendMode::Opaque,
		    { .BlendEnable = FALSE,
		        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL } },
		{ EBlendMode::Masked,
		    { .BlendEnable = FALSE,
		        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL } },
		{ EBlendMode::Translucent,
		    { .BlendEnable = TRUE,
		        .SrcBlend = D3D11_BLEND_SRC_ALPHA,
		        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
		        .BlendOp = D3D11_BLEND_OP_ADD,
		        .SrcBlendAlpha = D3D11_BLEND_ONE,
		        .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
		        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
		        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL } },
		{ EBlendMode::Additive,
		    { .BlendEnable = TRUE,
		        .SrcBlend = D3D11_BLEND_ONE,
		        .DestBlend = D3D11_BLEND_ONE,
		        .BlendOp = D3D11_BLEND_OP_ADD,
		        .SrcBlendAlpha = D3D11_BLEND_ONE,
		        .DestBlendAlpha = D3D11_BLEND_ZERO,
		        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
		        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL } },
		{ EBlendMode::PremultipliedAlpha,
		    { .BlendEnable = TRUE,
		        .SrcBlend = D3D11_BLEND_ONE,
		        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
		        .BlendOp = D3D11_BLEND_OP_ADD,
		        .SrcBlendAlpha = D3D11_BLEND_ONE,
		        .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
		        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
		        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL } },
	};

	D3D11_BLEND_DESC NativeDesc{};
	NativeDesc.RenderTarget[0] = BlendModeMap.at(Desc.BlendMode);

	Microsoft::WRL::ComPtr<ID3D11BlendState> State;

	if (FAILED(Device->CreateBlendState(&NativeDesc, &State)))
	{
		return nullptr;
	}

	BlendStateMap.emplace(Desc, State);
	return State;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState>
FRenderer::GetOrCreateSamplerState(const FTextureSamplerDesc& Desc)
{
	if (const auto It = SamplerStateMap.find(Desc); It != SamplerStateMap.end())
	{
		return It->second;
	}

	static const TMap<ETextureSamplerFilterMode, D3D11_FILTER> FilterModeMap{
		{ ETextureSamplerFilterMode::Point, D3D11_FILTER_MIN_MAG_MIP_POINT },
		{ ETextureSamplerFilterMode::Bilinear, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT },
		{ ETextureSamplerFilterMode::Trilinear, D3D11_FILTER_MIN_MAG_MIP_LINEAR },
		{ ETextureSamplerFilterMode::Anisotropic, D3D11_FILTER_ANISOTROPIC },
	};
	static const TMap<ETextureSamplerWrapMode, D3D11_TEXTURE_ADDRESS_MODE>
	    WrapModeMap{
		    { ETextureSamplerWrapMode::Wrap, D3D11_TEXTURE_ADDRESS_WRAP },
		    { ETextureSamplerWrapMode::Mirror, D3D11_TEXTURE_ADDRESS_MIRROR },
		    { ETextureSamplerWrapMode::Clamp, D3D11_TEXTURE_ADDRESS_CLAMP },
	    };
	static const TMap<ETextureSamplerFilterMode, UINT> MaxAnisotropyMap{
		{ ETextureSamplerFilterMode::Point, 1u },
		{ ETextureSamplerFilterMode::Bilinear, 1u },
		{ ETextureSamplerFilterMode::Trilinear, 1u },
		{ ETextureSamplerFilterMode::Anisotropic, 16u },
	};

	const D3D11_TEXTURE_ADDRESS_MODE Address = WrapModeMap.at(Desc.WrapMode);
	const D3D11_SAMPLER_DESC NativeDesc{
		.Filter = FilterModeMap.at(Desc.FilterMode),
		.AddressU = Address,
		.AddressV = Address,
		.AddressW = Address,
		.MaxAnisotropy = MaxAnisotropyMap.at(Desc.FilterMode),
		.ComparisonFunc = D3D11_COMPARISON_NEVER,
		.MaxLOD = D3D11_FLOAT32_MAX,
	};

	Microsoft::WRL::ComPtr<ID3D11SamplerState> State;
	if (FAILED(Device->CreateSamplerState(&NativeDesc, &State)))
	{
		return nullptr;
	}
	SamplerStateMap.emplace(Desc, State);
	return State;
}

TSharedPtr<FTexture> FRenderer::CreateTexture(const wchar_t* path)
{
	auto Texture = TSharedPtr<FTexture>{ new FTexture() };
	Microsoft::WRL::ComPtr<ID3D11Resource> TempResource;

	// dds first
	HRESULT hr = DirectX::CreateDDSTextureFromFile(Device.Get(), path, TempResource.GetAddressOf(), Texture->TextureSRV.GetAddressOf());

	// If dds failed
	if (FAILED(hr))
	{
		// Grayscale images are otherwise created as R8_UNORM. Sampling an R8
		// texture as RGBA yields (gray, 0, 0, 1), which makes grayscale base
		// color textures appear red. Convert WIC textures to RGBA explicitly.
		hr = DirectX::CreateWICTextureFromFileEx(
		    Device.Get(), path, 0,
		    D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
		    DirectX::WIC_LOADER_FORCE_RGBA32,
		    TempResource.GetAddressOf(), Texture->TextureSRV.GetAddressOf());
	}

	hr = TempResource.As(&Texture->Texture2D);
	if (FAILED(hr))
	{
		return nullptr;
	}

	D3D11_TEXTURE2D_DESC desc;
	Texture->Texture2D->GetDesc(&desc);
	Texture->Width = desc.Width;
	Texture->Height = desc.Height;
	Texture->Format = desc.Format;
	Texture->MipLevels = desc.MipLevels;
	Texture->MemorySize = Texture->GetMemorySize();

	INC_MEMORY_STAT_BY("TextureMemory", Texture->MemorySize);

	return Texture;
}

TSharedPtr<FTexture> FRenderer::CreateSolidTexture(const FVector4& Color)
{
	auto Texture = TSharedPtr<FTexture>{ new FTexture() };

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	uint8_t pixel[4] = {
		static_cast<uint8_t>(std::clamp(Color.X, 0.0f, 1.0f) * 255.0f),
		static_cast<uint8_t>(std::clamp(Color.Y, 0.0f, 1.0f) * 255.0f),
		static_cast<uint8_t>(std::clamp(Color.Z, 0.0f, 1.0f) * 255.0f),
		static_cast<uint8_t>(std::clamp(Color.W, 0.0f, 1.0f) * 255.0f)
	};

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = pixel;
	InitData.SysMemPitch = 4;

	if (FAILED(Device->CreateTexture2D(&desc, &InitData, Texture->Texture2D.GetAddressOf())))
	{
		return nullptr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	if (FAILED(Device->CreateShaderResourceView(Texture->Texture2D.Get(), &srvDesc, Texture->TextureSRV.GetAddressOf())))
	{
		return nullptr;
	}

	Texture->Width = 1;
	Texture->Height = 1;
	Texture->Format = desc.Format;
	Texture->MipLevels = 1;

	return Texture;
}

TSharedPtr<FRenderPipeline> FRenderer::GetPipeline(const FName& Id) const
{
	return FRenderResourceLibrary::Get().GetPipeline(Id);
}

void FRenderer::ClearLastRenderState()
{
	LastMesh = nullptr;
	LastTexture = nullptr;
	bHasLastTexture = false;
	LastRenderPipeline = nullptr;
}

bool FRenderer::InitializeDeviceAndSwapChain(HWND Window)
{
	constexpr D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	DXGI_SWAP_CHAIN_DESC SwapChainDesc{
		.BufferDesc = {
		    .Width = 0u,
		    .Height = 0u,
		    .Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		},
		.SampleDesc = {
		    .Count = 1u,
		},
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = 2u,
		.OutputWindow = Window,
		.Windowed = true,
		.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
	};

	UINT CreateDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifndef NDEBUG
	CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT Result = D3D11CreateDeviceAndSwapChain(
	    nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, CreateDeviceFlags,
	    FeatureLevels, ARRAYSIZE(FeatureLevels), D3D11_SDK_VERSION,
	    &SwapChainDesc, &SwapChain, &Device, nullptr, &Context);
	if (FAILED(Result))
	{
		return false;
	}

	Result = Context.As(&Context1);

	if (FAILED(Result))
	{
		Context1.Reset();
		return false;
	}

	RECT ClientRect{};
	GetClientRect(Window, &ClientRect);

	Viewport = {
		.TopLeftX = 0.0f,
		.TopLeftY = 0.0f,
		.Width = static_cast<float>(ClientRect.right - ClientRect.left),
		.Height = static_cast<float>(ClientRect.bottom - ClientRect.top),
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f,
	};

	return true;
}

bool FRenderer::InitializeBackBuffer()
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBuffer;
	HRESULT Result = SwapChain->GetBuffer(0, IID_PPV_ARGS(&BackBuffer));
	if (FAILED(Result))
	{
		return false;
	}

	Result = Device->CreateRenderTargetView(BackBuffer.Get(), nullptr, &BackBufferRTV);
	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

bool FRenderer::InitializePointLightBuffers()
{
	D3D11_BUFFER_DESC LightDesc{};
	LightDesc.ByteWidth =
	    sizeof(FPointLightConstants) * MaxPointLightCount;
	LightDesc.Usage = D3D11_USAGE_DEFAULT;
	LightDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	LightDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	LightDesc.StructureByteStride = sizeof(FPointLightConstants);

	HRESULT Result = Device->CreateBuffer(
	    &LightDesc, nullptr, PointLightBuffer.GetAddressOf());

	if (FAILED(Result))
	{
		return false;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc{};
	SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SrvDesc.Buffer.FirstElement = 0;
	SrvDesc.Buffer.NumElements = MaxPointLightCount;

	Result = Device->CreateShaderResourceView(
	    PointLightBuffer.Get(),
	    &SrvDesc,
	    PointLightSRV.GetAddressOf());

	if (FAILED(Result))
	{
		return false;
	}

	FPointLightCountConstants InitialCount{};

	D3D11_BUFFER_DESC CountDesc{};
	CountDesc.ByteWidth = sizeof(FPointLightCountConstants);
	CountDesc.Usage = D3D11_USAGE_DEFAULT;
	CountDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA InitialData{};
	InitialData.pSysMem = &InitialCount;

	Result = Device->CreateBuffer(
	    &CountDesc,
	    &InitialData,
	    PointLightCountBuffer.GetAddressOf());

	return SUCCEEDED(Result);
}

// 현재 깊이 버퍼 기준으로 각 명령이 실제로 보이는 픽셀 수를 GPU에 묻는다.
// GPU가 끝날 때까지 기다리므로 느리다. 디버깅에서 쓰는 한 프레임 측정 전용
void FRenderer::QueryVisibility(const TArray<const FDrawCommand*>& Commands, TArray<uint64>& OutSamples)
{
	OutSamples.assign(Commands.size(), 0);

	if (OutSamples.empty())
		return;

	// 측정용 리소스 생성
	if (!OracleDepthState)
	{
		D3D11_DEPTH_STENCIL_DESC DepthDesc{};
		DepthDesc.DepthEnable = TRUE;
		// 측정이 깊이 버퍼를 바꾸면 안 된다
		DepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		// 자기 자신과 같은 깊이는 통과 → 그려진 오브젝트는 "보임"
		DepthDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		DepthDesc.StencilEnable = FALSE;
		HRESULT hr = Device->CreateDepthStencilState(&DepthDesc, &OracleDepthState);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("[Oracle] 오클루전 DepthStencilState 생성 실패");
			return;
		}

		D3D11_BLEND_DESC BlendDesc{};
		// 화면 색은 건드리지 않는다
		BlendDesc.RenderTarget[0].RenderTargetWriteMask = 0;
		hr = Device->CreateBlendState(&BlendDesc, &OracleBlendState);
		if (FAILED(hr))
		{
			UE_LOG_ERROR("[Oracle] 오클루전 BlendState 생성 실패");
			return;
		}
	}

	// 쿼리 객체 준비
	const D3D11_QUERY_DESC QueryDesc{ D3D11_QUERY_OCCLUSION, 0 };
	while (OracleQueries.size() < Commands.size())
	{
		Microsoft::WRL::ComPtr<ID3D11Query> Query;
		if (FAILED(Device->CreateQuery(&QueryDesc, &Query)))
		{
			UE_LOG_ERROR("[Oracle] 오클루전 쿼리 생성 실패");
			return;
		}
		OracleQueries.push_back(Query);
	}

	// 명령마다: 원래와 같은 파이프라인(같은 정점 셰이더, 같은 깊이)으로 그리되
	// 깊이/블렌드 상태만 측정용으로 덮어쓴다
	for (size_t i = 0; i < Commands.size(); ++i)
	{
		const FDrawCommand& Command = *Commands[i];
		if (!Command.Mesh || Command.Materials.empty())
		{
			continue;
		}

		// 오브젝트 constant buffer 업데이트
		UpdateBuffer(Command.Constants, 2);

		const FMaterial& Material = Command.Materials[0];
		if (FRenderPipeline* Pipeline = Material.GetPipeline())
		{
			Pipeline->Bind(*Context.Get());
		}
		// Material.BindResources(*Context.Get());
		Command.Mesh->BindResources(*Context.Get());

		Context->OMSetDepthStencilState(OracleDepthState.Get(), 0);
		Context->OMSetBlendState(OracleBlendState.Get(), nullptr, 0xFFFFFFFF);

		Context->Begin(OracleQueries[i].Get());
		if (Command.Mesh->HasIndices())
		{
			Context->DrawIndexed(Command.Mesh->IndexCount, 0, 0);
		}
		else
		{
			Context->Draw(Command.Mesh->VertexCount, 0);
		}
		Context->End(OracleQueries[i].Get());
	}

	// 결과 회수: GPU가 끝날 때까지 기다린다 (측정 전용이라 멈춤을 허용)
	for (size_t i = 0; i < Commands.size(); ++i)
	{
		UINT64 Samples = 0;
		HRESULT Result = S_FALSE;
		do
		{
			Result = Context->GetData(OracleQueries[i].Get(), &Samples, sizeof(Samples), 0);
		} while (Result == S_FALSE);

		OutSamples[i] = SUCCEEDED(Result) ? Samples : 0;
	}

	// 상태를 직접 바인딩했으므로 렌더 상태 캐시를 무효화
	ClearLastRenderState();
}

void FRenderer::BindPointLights()
{
	ID3D11ShaderResourceView* SRV = PointLightSRV.Get();
	Context->PSSetShaderResources(3, 1, &SRV);

	ID3D11Buffer* Buffer = PointLightCountBuffer.Get();
	Context->PSSetConstantBuffers(5, 1, &Buffer);
}



// =====================================================================
// ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄ Constants Buffer ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄

bool FRenderer::InitializeConstantBuffers()
{
	// Dynamic Constant Buffer
	// 여러개의 Constant Buffer를 하나로 묶어서 사용
	// Map, Copy, Unmap 횟수 줄이기.

	D3D11_BUFFER_DESC Desc{};
	Desc.ByteWidth = ObjectConstantUploadBufferSize;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.StructureByteStride = 0;

	HRESULT Result = Device->CreateBuffer(&Desc, nullptr, &ObjectConstantUploadBuffer);

	if (FAILED(Result))
	{
		return false;
	}

	D3D11_BUFFER_DESC LightConstantBufferDesc = {
		.ByteWidth = sizeof(FLightConstants),
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
	};

	Result = Device->CreateBuffer(&LightConstantBufferDesc, nullptr, &LightConstantBuffer);

	if (FAILED(Result))
	{
		return false;
	}

	for (int32 i = 0; i < NumFrameResourceCount; i++)
	{
		D3D11_BUFFER_DESC FrameResourceConstantBufferDesc = {
			.ByteWidth = sizeof(FFrameConstants),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		};
		Result = Device->CreateBuffer(&FrameResourceConstantBufferDesc, nullptr, &FrameResources[i].FrameConstantBuffer);
		if (FAILED(Result))
		{
			return false;
		}

		D3D11_BUFFER_DESC FrameResourceViewConstantBufferDesc = {
			.ByteWidth = sizeof(FViewConstants),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		};
		Result = Device->CreateBuffer(&FrameResourceViewConstantBufferDesc, nullptr, &FrameResources[i].ViewConstantBuffer);
		if (FAILED(Result))
		{
			return false;
		}

		D3D11_BUFFER_DESC FrameResourceObjectConstantBufferDesc = {
			.ByteWidth = ConstantBufferSize,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
		};
		Result = Device->CreateBuffer(&FrameResourceObjectConstantBufferDesc, nullptr, &FrameResources[i].ObjectConstantBuffer);
		if (FAILED(Result))
		{
			return false;
		}

		D3D11_BUFFER_DESC FrameResourcePostProcessConstantBufferDesc = {
			.ByteWidth = sizeof(FPostProcessConstants),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		};
		Result = Device->CreateBuffer(&FrameResourcePostProcessConstantBufferDesc, nullptr, &FrameResources[i].PostProcessConstantBuffer);
		if (FAILED(Result))
		{
			return false;
		}

		D3D11_BUFFER_DESC FrameResourceHeightFogConstantBufferDesc = {
			.ByteWidth = sizeof(FHeightFogConstants),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		};
		Result = Device->CreateBuffer(&FrameResourceHeightFogConstantBufferDesc, nullptr, &FrameResources[i].HeightFogConstantBuffer);
		D3D11_BUFFER_DESC FXAABufferDesc = {
			.ByteWidth = sizeof(FFXAAConstants),
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
		};
		Result = Device->CreateBuffer(&FXAABufferDesc, nullptr, &FrameResources[i].FXAAConstantBuffer);

		if (FAILED(Result))
		{
			return false;
		}
	}

	return true;
}

void FRenderer::UpdateLightConstants(const FLightConstants& Constants)
{
	Context->UpdateSubresource(LightConstantBuffer.Get(), 0, nullptr, &Constants, 0, 0);
	Context->PSSetConstantBuffers(4, 1, LightConstantBuffer.GetAddressOf());
}

void FRenderer::UpdateFrameConstants(const FFrameConstants& Constants)
{
	Context->UpdateSubresource(GetCurrentFrameResource()->FrameConstantBuffer.Get(), 0, nullptr, &Constants, 0, 0);
	Context->VSSetConstantBuffers(0, 1, GetCurrentFrameResource()->FrameConstantBuffer.GetAddressOf());
	Context->PSSetConstantBuffers(0, 1, GetCurrentFrameResource()->FrameConstantBuffer.GetAddressOf());
}

void FRenderer::UpdateViewConstants(const FViewConstants& Constants)
{
	ViewProjection = Constants.View * Constants.Projection;
	Context->UpdateSubresource(GetCurrentFrameResource()->ViewConstantBuffer.Get(), 0, nullptr, &Constants, 0, 0);
	Context->VSSetConstantBuffers(1, 1, GetCurrentFrameResource()->ViewConstantBuffer.GetAddressOf());
	Context->PSSetConstantBuffers(1, 1, GetCurrentFrameResource()->ViewConstantBuffer.GetAddressOf());
}

void FRenderer::UpdateViewConstants(const FCamera& Camera, FVector2 ViewportSizePixel)
{
	FMatrix Projection = Camera.GetProjectionMatrix();
	FMatrix ProjectionD3D = Projection.ToD3DMatrix();
	FMatrix ViewProjectionD3D = Camera.GetViewMatrix() * ProjectionD3D;
	FMatrix InverseVPD3D;
	ViewProjectionD3D.Inverse(InverseVPD3D);

	UpdateViewConstants(FViewConstants{
		.View = Camera.GetViewMatrix(),
		.Projection = ProjectionD3D,
		.ViewProjectionInverse = InverseVPD3D,
		.ViewportSize = ViewportSizePixel,
		.NearZ = Camera.GetProjection().GetNearPlane(),
		.FarZ = Camera.GetProjection().GetFarPlane(),
		.IsPerspective = Camera.GetProjection().GetProjectionType() == EProjectionType::Perspective ? 1.f : 0.f,
		.CameraPos = Camera.GetPosition(),
	});
}

void FRenderer::UpdatePostProcessConstants(const FPostProcessConstants& Constants)
{
	Context->UpdateSubresource(GetCurrentFrameResource()->PostProcessConstantBuffer.Get(), 0, nullptr, &Constants, 0, 0);
	Context->PSSetConstantBuffers(3, 1, GetCurrentFrameResource()->PostProcessConstantBuffer.GetAddressOf());
}

void FRenderer::UpdateHeightFogConstants(const FHeightFogConstants& Constants)
{
	Context->UpdateSubresource(GetCurrentFrameResource()->HeightFogConstantBuffer.Get(), 0, nullptr, &Constants, 0, 0);
	Context->PSSetConstantBuffers(7, 1, GetCurrentFrameResource()->HeightFogConstantBuffer.GetAddressOf());
}

bool FRenderer::UploadObjectConstants(std::span<const FDrawCommand> Commands)
{
	if (Commands.empty())
	{
		return true;
	}
	if (Commands.size() > MaxObjectDrawCount)
	{
		return false;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped{};

	HRESULT Result = Context->Map(ObjectConstantUploadBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	if (FAILED(Result))
	{
		return false;
	}

	std::byte* Destination = static_cast<std::byte*>(Mapped.pData);

	for (size_t Index = 0; Index < Commands.size(); ++Index)
	{
		const uint32 ByteOffset = static_cast<uint32>(Index) * ObjectConstantStride;
		FObjectConstants ShaderConstants = Commands[Index].Constants;
		//ShaderConstants.MVP = (ShaderConstants.World * ViewProjection).ToD3DMatrix();
		std::memcpy(Destination + ByteOffset, &ShaderConstants, sizeof(FObjectConstants));
	}
	Context->Unmap(ObjectConstantUploadBuffer.Get(), 0);

	return true;
}

void FRenderer::BindObjectConstantRange(uint32 Slot, uint32 ByteOffset)
{
	assert(Context1);
	assert(ByteOffset % 256u == 0);

	ID3D11Buffer* Buffer = ObjectConstantUploadBuffer.Get();

	// SetConstantBuffers1의 단위는 16바이트 shader constant.
	UINT FirstConstant = ByteOffset / 16u;
	UINT NumConstants = ObjectConstantStride / 16u;

	Context1->VSSetConstantBuffers1(Slot, 1, &Buffer, &FirstConstant, &NumConstants);
	Context1->PSSetConstantBuffers1(Slot, 1, &Buffer, &FirstConstant, &NumConstants);
}

void FRenderer::UploadPointLights(std::span<const FPointLightConstants> PointLights)
{
	const uint32 Count = PointLights.size() > MaxPointLightCount
	                         ? MaxPointLightCount
	                         : static_cast<uint32>(PointLights.size());

	// Update the entire fixed-size buffer from equally sized CPU storage.
	// Passing the collected array directly would be too short when Count < 64.
	FPointLightConstants UploadData[MaxPointLightCount]{};
	for (uint32 i = 0; i < Count; ++i)
	{
		UploadData[i] = PointLights[i];
	}

	Context->UpdateSubresource(PointLightBuffer.Get(), 0, nullptr, UploadData, 0, 0);

	FPointLightCountConstants CountData{};
	CountData.PointLightCount = Count;
	Context->UpdateSubresource(PointLightCountBuffer.Get(), 0, nullptr, &CountData, 0, 0);
}

void FRenderer::UpdateFXAAConstants(const FFXAAConstants& Constants)
{
	ID3D11Buffer* Buffer = GetCurrentFrameResource()->FXAAConstantBuffer.Get();

	Context->UpdateSubresource(Buffer, 0, nullptr, &Constants, 0, 0);

	Context->PSSetConstantBuffers(6, 1, &Buffer);
}

// ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃ Constants Buffer ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃
// =====================================================================


// =====================================================================
// ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄ Draw ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄ 

// OverridePipeline이 있으면 그 파이프라인을 쓰고 없으면 nullptr
void FRenderer::Draw(const FDrawCommand& Command, FRenderPipeline* OverridePipeline, uint32 Slot)
{
	if (!Command.Mesh || Command.Materials.empty())
	{
		return;
	}

	if (!Command.Mesh->Sections.empty())
	{
		for (size_t i = 0; i < Command.Mesh->Sections.size(); i++)
		{
			const auto& Section = Command.Mesh->Sections[i];

			const FMaterial& Mat = (i < Command.Materials.size()) ? Command.Materials[i] : Command.Materials[0];

			DrawSection(*Command.Mesh, Mat, Command.Constants, OverridePipeline, Section.StartIndex, Section.IndexCount, Slot);
		}
	}
	else
	{
		Draw(*Command.Mesh, Command.Materials[0], Command.Constants, OverridePipeline, Slot);
	}
}

void FRenderer::DrawPrimitiveBatch(std::span<const FDrawCommand> Commands, FRenderPipeline* OverridePipeline)
{
	size_t Begin = 0;

	while (Begin < Commands.size())
	{
		const size_t Remaining = Commands.size() - Begin;
		const size_t ChunkCount = std::min(Remaining, static_cast<size_t>(MaxObjectDrawCount));

		std::span<const FDrawCommand> Chunk = Commands.subspan(Begin, ChunkCount);
		if (!UploadObjectConstants(Chunk))
		{
			throw EngineUtil::CreateError("ObjectConstantUploadBuffer Map에 실패했습니다.");
		}

		for (size_t LocalIndex = 0; LocalIndex < Chunk.size(); ++LocalIndex)
		{
			const uint32 ByteOffset = static_cast<uint32>(LocalIndex) * ObjectConstantStride;
			BindObjectConstantRange(2, ByteOffset);
			DrawUploadedCommand(Chunk[LocalIndex], OverridePipeline);
		}
		Begin += ChunkCount;
	}
}

void FRenderer::BindDrawResources(const FMesh& Mesh, const FMaterial& Material, FRenderPipeline* OverridePipeline)
{
	// OverridePipeline이 있으면 그걸 쓰고 없으면 Material의 파이프라인을 쓴다
	FRenderPipeline* Pipeline = OverridePipeline ? OverridePipeline : Material.GetPipeline();
	FTexture* Texture = Material.GetTexture();

	if (LastRenderPipeline != Pipeline)
	{
		if (Pipeline)
		{
			Pipeline->Bind(*Context.Get());
		}
		LastRenderPipeline = Pipeline;
	}

	if (!bHasLastTexture || LastTexture != Texture)
	{
		ID3D11ShaderResourceView* SRV = Texture ? Texture->GetSRV() : nullptr;
		Context->PSSetShaderResources(0u, 1u, &SRV);
		LastTexture = Texture;
		bHasLastTexture = true;
	}

	if (LastMesh != &Mesh)
	{
		Mesh.BindResources(*Context.Get());
		LastMesh = &Mesh;
	}
}

void FRenderer::DrawUploadedCommand(const FDrawCommand& Command, FRenderPipeline* OverridePipeline)
{
	if (!Command.Mesh || Command.Materials.empty())
	{
		return;
	}

	const FMesh& Mesh = *Command.Mesh;

	if (!Mesh.Sections.empty())
	{
		size_t SectionSize = Mesh.Sections.size();
		for (size_t SectionIndex = 0; SectionIndex < SectionSize; ++SectionIndex)
		{
			const FMeshSection& Section = Mesh.Sections[SectionIndex];
			const FMaterial& Material = SectionIndex < Command.Materials.size() ? Command.Materials[SectionIndex] : Command.Materials[0];

			BindDrawResources(Mesh, Material, OverridePipeline);
			Context->DrawIndexed(Section.IndexCount, Section.StartIndex, 0);

			INC_DWORD_STAT_BY("Prims", Section.IndexCount / 3u);
			INC_DWORD_STAT("Draws");
		}
	}
	else
	{
		const FMaterial& Material = Command.Materials[0];
		BindDrawResources(Mesh, Material, OverridePipeline);

		if (Mesh.HasIndices())
		{
			Context->DrawIndexed(Mesh.IndexCount, 0, 0);
			INC_DWORD_STAT_BY("Prims", Mesh.IndexCount / 3u);
		}
		else
		{
			Context->Draw(Mesh.VertexCount, 0);
			INC_DWORD_STAT_BY("Prims", Mesh.VertexCount / 3u);
		}
		INC_DWORD_STAT("Draws");
	}
}

// ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃ Draw ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃
// =====================================================================







// =====================================================================
// ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄ Text ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄ 

void FRenderer::AddTextInstanceArray(const FDrawCommand& Command)
{
	// 빈 데이터 전달 시 조기 반환
	if (!Command.Mesh || Command.Materials.empty() || Command.Instances.empty())
	{
		return;
	}
	auto& ResLib = FRenderResourceLibrary::Get();

	auto& TargetArray = ResLib.GetInstancingArray(Command.Mesh, &Command.Materials[0]);
	TargetArray.reserve(TargetArray.size() + Command.Instances.size());
	TargetArray.insert(TargetArray.end(), Command.Instances.begin(), Command.Instances.end());
}

void FRenderer::DrawInstances(const FCamera& Camera, FRenderPipeline* OverridePipeline, bool bDisableShading)
{
	auto& ResLib = FRenderResourceLibrary::Get();

	FObjectConstants SC{};
	SC.World = FMatrix::Identity;

	// 배치 키(MaterialID, MeshID) 순회
	for (const auto& [BatchKey, InstanceData] : ResLib.AllInstancingArrayMap)
	{
		if (InstanceData.empty())
			continue;

		SC.DisableShading = bDisableShading ? 1.0f : 0.0f;
		UpdateBuffer(SC, 2);

		const UINT InstanceCount = static_cast<UINT>(InstanceData.size());
		const UINT RequiredSize = InstanceCount * sizeof(FInstanceData);

		// 버퍼 크기 부족 시 동적 확장
		if (RequiredSize > TextInstanceBufferSize)
		{
			const size_t OldSize = TextInstanceBufferSize;
			Microsoft::WRL::ComPtr<ID3D11Buffer> NewBuffer;

			// InstanceBuffer.Reset();
			D3D11_BUFFER_DESC Desc{};
			Desc.ByteWidth = RequiredSize;
			Desc.Usage = D3D11_USAGE_DYNAMIC;
			Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			if (FAILED(Device->CreateBuffer(&Desc, nullptr, &NewBuffer)))
			{
				continue;
			}
			InstanceBuffer = NewBuffer;
			TextInstanceBufferSize = RequiredSize;
		}

		// 인스턴스 데이터 업로드
		D3D11_MAPPED_SUBRESOURCE MappedResource{};
		if (FAILED(Context->Map(InstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
			continue;
		std::memcpy(MappedResource.pData, InstanceData.data(), RequiredSize);
		Context->Unmap(InstanceBuffer.Get(), 0);

		// 머티리얼의 리소스 값을 렌더러가 캐싱하여 바인딩
		const FMaterial* Material = BatchKey.Material;
		if (!Material)
			continue;

		// 메시 조회 및 바인딩
		const FMesh* Mesh = BatchKey.Mesh;
		if (!Mesh)
			continue;
		BindDrawResources(*Mesh, *Material, nullptr);

		// 슬롯 1에 인스턴스 버퍼 바인딩
		UINT Stride = sizeof(FInstanceData);
		UINT Offset = 0;
		Context->IASetVertexBuffers(1, 1, InstanceBuffer.GetAddressOf(), &Stride, &Offset);

		// 인스턴스 렌더링 호출
		if (Mesh->HasIndices())
		{
			Context->DrawIndexedInstanced(Mesh->GetIndexCount(), InstanceCount, 0, 0, 0);
			INC_DWORD_STAT_BY("Prims", Mesh->GetIndexCount() / 3u * InstanceCount);
		}
		else
		{
			Context->DrawInstanced(Mesh->VertexCount, InstanceCount, 0, 0);
			INC_DWORD_STAT_BY("Prims", Mesh->VertexCount / 3u * InstanceCount);
		}
		INC_DWORD_STAT("Draws");
	}
}

void FRenderer::DrawTextInstances(const FDrawCommand& Command)
{
	if (!Command.Mesh || Command.Materials.empty())
	{
		return;
	}

	auto& ResLib = FRenderResourceLibrary::Get();

	const TArray<FInstanceData>& InstanceData = ResLib.GetInstancingArray(Command.Mesh, &Command.Materials[0]);
	DrawTextInstanceData(Command, InstanceData);
}

void FRenderer::DrawTextInstanceData(const FDrawCommand& Command, std::span<const FInstanceData> InstanceData)
{
	if (!Command.Mesh || Command.Materials.empty() || InstanceData.empty())
	{
		return;
	}
	UpdateBuffer(Command.Constants, 2);

	const UINT InstanceCount = static_cast<UINT>(InstanceData.size());
	const UINT RequiredSize = InstanceCount * sizeof(FInstanceData);

	// 버퍼 크기 부족 시 동적 확장
	if (RequiredSize > TextInstanceBufferSize)
	{
		const size_t OldSize = TextInstanceBufferSize;
		Microsoft::WRL::ComPtr<ID3D11Buffer> NewBuffer;

		// InstanceBuffer.Reset();
		D3D11_BUFFER_DESC Desc{};
		Desc.ByteWidth = RequiredSize;
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if (FAILED(Device->CreateBuffer(&Desc, nullptr, &NewBuffer)))
		{
			return;
		}
		InstanceBuffer = NewBuffer;
		TextInstanceBufferSize = RequiredSize;
	}

	// 인스턴스 데이터 업로드
	D3D11_MAPPED_SUBRESOURCE MappedResource{};
	if (FAILED(Context->Map(InstanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)))
		return;
	std::memcpy(MappedResource.pData, InstanceData.data(), RequiredSize);
	Context->Unmap(InstanceBuffer.Get(), 0);

	// 머티리얼의 리소스 값을 렌더러가 캐싱하여 바인딩
	const FMaterial* Material = &Command.Materials[0];

	// 메시 조회 및 바인딩
	const FMesh* Mesh = Command.Mesh;
	BindDrawResources(*Mesh, *Material, nullptr);

	// 슬롯 1에 인스턴스 버퍼 바인딩
	UINT Stride = sizeof(FInstanceData);
	UINT Offset = 0;
	Context->IASetVertexBuffers(1, 1, InstanceBuffer.GetAddressOf(), &Stride, &Offset);

	// 인스턴스 렌더링 호출
	if (Mesh->HasIndices())
	{
		Context->DrawIndexedInstanced(Mesh->GetIndexCount(), InstanceCount, 0, 0, 0);
		INC_DWORD_STAT_BY("Prims", Mesh->GetIndexCount() / 3u * InstanceCount);
	}
	else
	{
		Context->DrawInstanced(Mesh->VertexCount, InstanceCount, 0, 0);
		INC_DWORD_STAT_BY("Prims", Mesh->VertexCount / 3u * InstanceCount);
	}
	INC_DWORD_STAT("Draws");
}

void FRenderer::DrawBillboardText(const FSceneView& View, FWStringView Text,
    const FVector& WorldPosition, float WorldSize)
{
	const FViewportRenderTarget* Target = View.Viewport.RenderTarget.get();
	if (!Target || !ActiveSceneTextures || Text.empty() || WorldSize <= 0.0f)
	{
		return;
	}

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	UFont* FontAsset = Registry.Get<UFont>("Font/NanumGothicBold.json");
	UMaterial* MaterialAsset = Registry.Get<UMaterial>("Material/SelectedActor_Text.json");
	UStaticMesh* MeshAsset = Registry.Get<UStaticMesh>("#Rect");
	if (!FontAsset || !FontAsset->Get() || !FontAsset->GetTexture() ||
	    !MaterialAsset || !MaterialAsset->GetPipeline() || !MeshAsset || !MeshAsset->Get())
	{
		return;
	}

	const FFont* Font = FontAsset->Get();
	if (CachedBillboardText != Text || CachedBillboardFont != Font)
	{
		float Width, Height;
		TextRendering::BuildGlyphInstances(Text, *Font, FVector4{1.0f, 1.0f, 1.0f, 1.0f},
		    CachedBillboardGlyphs, Width, Height);
		CachedBillboardText = Text;
		CachedBillboardFont = Font;
	}

	FTransform Transform;
	Transform.SetLocation(WorldPosition);
	Transform.SetScale3D(FVector{WorldSize, WorldSize, WorldSize});
	const FMatrix Billboard = BillboardRendering::MakeBillboardMatrix(Transform, View.Camera);
	BillboardWorldInstances.clear();
	BillboardWorldInstances.reserve(CachedBillboardGlyphs.size());
	for (const FInstanceData& Glyph : CachedBillboardGlyphs)
	{
		FInstanceData Instance = Glyph;
		Instance.World *= Billboard;
		BillboardWorldInstances.push_back(Instance);
	}

	FMaterial Material;
	Material.SetPipeLine(MaterialAsset->GetPipeline()->Get());
	Material.SetTexture(FontAsset->GetTexture()->Get());
	Material.SetSamplerDesc(MaterialAsset->GetSamplerDesc());
	FDrawCommand Command;
	Command.Mesh = MeshAsset->Get();
	Command.Materials = std::span<const FMaterial>(&Material, 1);
	Command.Type = ERenderType::Text;
	// 글자 인스턴스에 월드 변환이 이미 적용되어 있으므로 World는 Identity.
	Command.Constants.World = FMatrix::Identity;

	ClearLastRenderState();
	BindRenderTarget(Target->GetRTV(), ActiveSceneTextures->SceneDepthDSV.Get());
	SetViewportPixel(View.ViewportSizePixel);
	UpdateViewConstants(View.Camera, View.ViewportSizePixel);
	DrawTextInstanceData(Command, BillboardWorldInstances);
	ClearLastRenderState();
}

void FRenderer::ClearTextInstances()
{
	for (auto& [BatchKey, InstanceArray] : FRenderResourceLibrary::Get().AllInstancingArrayMap)
	{
		InstanceArray.clear();
	}

	FRenderResourceLibrary::Get().DestroyAllInstancingArray();
}

// 풀스크린 패스 공통 처리 (Draw(3,0))
// 후처리 규칙: 후처리 체인의 마지막 패스는 반드시 뷰포트 출력 RT 전체를 쓴다.
void FRenderer::DrawScreenPass(ID3D11RenderTargetView* TargetRTV, const D3D11_VIEWPORT& TargetD3DViewport,
    ID3D11ShaderResourceView* const* SRVs, UINT NumSRVs, const FName& PipelineId)
{
	if (!TargetRTV)
	{
		return;
	}

	TSharedPtr<FRenderPipeline> Pipeline = FRenderResourceLibrary::Get().GetPipeline(PipelineId);
	if (!Pipeline)
	{
		return;
	}

	// SRV 최댓값 = 8
	constexpr UINT MaxScreenPassSRVs = 8u;
	NumSRVs = std::min(NumSRVs, MaxScreenPassSRVs);

	// 출력 대상 바인딩. 풀스크린 패스는 깊이를 쓰지 않으므로 DSV는 비운다 (같은 깊이 텍스처를 SRV로 읽을 수 있도록)
	BindRenderTarget(TargetRTV, nullptr);
	Context->RSSetViewports(1, &TargetD3DViewport);

	// ScreenQuadVS로 Full screen quad를 만든다
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Context->IASetInputLayout(nullptr);
	ID3D11Buffer* NullVB = nullptr;
	UINT Zero = 0;
	Context->IASetVertexBuffers(0, 1, &NullVB, &Zero, &Zero);

	if (NumSRVs > 0 && SRVs)
	{
		Context->PSSetShaderResources(0, NumSRVs, SRVs);
	}

	Pipeline->Bind(*Context.Get());
	Context->Draw(3, 0);
	INC_DWORD_STAT("Draws");
	INC_DWORD_STAT_BY("Prims", 1);

	// 슬롯 해제: 다음 패스에서 이 텍스처를 RTV로 바인딩할 수 있도록
	if (NumSRVs > 0)
	{
		ID3D11ShaderResourceView* NullSRVs[MaxScreenPassSRVs] = {};
		Context->PSSetShaderResources(0, NumSRVs, NullSRVs);
	}

	// 캐시 무효화
	ClearLastRenderState();
}

void FRenderer::RenderFXAA(ID3D11ShaderResourceView* InputSRV, ID3D11RenderTargetView* OutputRTV, UINT Width, UINT Height)
{
	if (!InputSRV || !OutputRTV || Width == 0 || Height == 0)
	{
		return;
	}

	const float InvWidth = 1.0f / static_cast<float>(Width);
	const float InvHeight = 1.0f / static_cast<float>(Height);

	FFXAAConstants Constants{};
	Constants.InvTextureSize = { InvWidth, InvHeight };

	// 텍스처 가장자리 픽셀의 중심까지 샘플링 허용.
	Constants.UVMin = { InvWidth * 0.5f, InvHeight * 0.5f };
	Constants.UVMax = {
		1.0f - InvWidth * 0.5f,
		1.0f - InvHeight * 0.5f
	};

	UpdateFXAAConstants(Constants);

	D3D11_VIEWPORT Viewport{};
	Viewport.Width = static_cast<float>(Width);
	Viewport.Height = static_cast<float>(Height);
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;

	ID3D11ShaderResourceView* SRVs[] = { InputSRV };

	DrawScreenPass(OutputRTV, Viewport, SRVs, 1, FName("#FXAA"));
}

void FRenderer::RenderViewportFXAA(const FViewport& TargetViewport)
{
	const FViewportRenderTarget* RenderTarget = TargetViewport.RenderTarget.get();
	if (!RenderTarget || !ActiveSceneTextures || !GetPipeline(FName("#FXAA")))
	{
		return;
	}

	const UINT Width = RenderTarget->GetWidth();
	const UINT Height = RenderTarget->GetHeight();
	if (Width == 0 || Height == 0 ||
		ActiveSceneTextures->Width != Width || ActiveSceneTextures->Height != Height)
	{
		return;
	}

	// Read the outlined viewport into a separate texture to avoid SRV/RTV aliasing.
	RenderFXAA(RenderTarget->GetSRV(), ActiveSceneTextures->SceneColorRTV.Get(), Width, Height);

	// Keep the viewport output used by gizmos and final composition up to date.
	CopySceneColorToViewport(TargetViewport);
}



// ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃ Draw ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃
// =====================================================================






// =====================================================================
// ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄ Post Process ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄

// SceneColor를 그대로 출력 RT에 복사
void FRenderer::CopySceneColorToViewport(const FViewport& TargetViewport)
{
	const FViewportRenderTarget* RenderTarget = TargetViewport.RenderTarget.get();
	if (!RenderTarget || !ActiveSceneTextures)
	{
		return;
	}

	const D3D11_VIEWPORT TargetD3DViewport = MakeD3DViewport(0.0f, 0.0f, static_cast<float>(RenderTarget->GetWidth()), static_cast<float>(RenderTarget->GetHeight()));

	// 합성용 파이프라인(CompositePS: t0을 UV로 샘플링)을 그대로 복사에 재사용한다
	ID3D11ShaderResourceView* SRVs[] = { ActiveSceneTextures->GetCurrentSRV() };
	DrawScreenPass(RenderTarget->GetRTV(), TargetD3DViewport, SRVs, 1, FName("#Composite"));
}

// SceneColor + Stencil을 읽어 외곽선을 그려 출력 RT에 그린다
void FRenderer::RenderSelectionOutline(const FViewport& TargetViewport)
{
	const FViewportRenderTarget* RenderTarget = TargetViewport.RenderTarget.get();
	if (!RenderTarget || !ActiveSceneTextures)
	{
		return;
	}

	// 출력 RT와 SceneTextures는 같은 크기이고 둘 다 (0,0)부터 시작한다.
	// 그래서 SelectionOutlinePS가 SV_Position으로 Load해도 좌표가 그대로 맞는다. 셈플러 필요 x
	const D3D11_VIEWPORT TargetD3DViewport = MakeD3DViewport(0.0f, 0.0f, static_cast<float>(RenderTarget->GetWidth()), static_cast<float>(RenderTarget->GetHeight()));

	// t0: SceneColor, t1: Stencil
	ID3D11ShaderResourceView* SRVs[] = { ActiveSceneTextures->GetCurrentSRV(), ActiveSceneTextures->SceneStencilSRV.Get() };
	DrawScreenPass(RenderTarget->GetRTV(), TargetD3DViewport, SRVs, 2, FName("#SelectionOutline"));
}

// Scene Depth를 출력 RT에 그린다
void FRenderer::RenderSceneDepth(const FViewport& TargetViewport)
{
	const FViewportRenderTarget* RenderTarget = TargetViewport.RenderTarget.get();
	if (!RenderTarget || !ActiveSceneTextures)
	{
		return;
	}

	const D3D11_VIEWPORT TargetD3DViewport = MakeD3DViewport(0.f, 0.f, TargetViewport.Rect.GetWidth(), TargetViewport.Rect.GetHeight());

	ID3D11ShaderResourceView* SRVs[] = { ActiveSceneTextures->SceneDepthSRV.Get() };
	//DrawScreenPass(RenderTarget->GetRTV(), TargetD3DViewport, SRVs, 1, FName("#SceneDepth"));
	DrawScreenPass(ActiveSceneTextures->GetTargetRTV(), TargetD3DViewport, SRVs, 1, FName("#SceneDepth"));
	ActiveSceneTextures->SwapPingPong();
}

// Fog를 SceneColor에 그린다
void FRenderer::RenderFog(const FViewport& TargetViewport)
{
	const FViewportRenderTarget* RenderTarget = TargetViewport.RenderTarget.get();
	if (!RenderTarget || !ActiveSceneTextures)
	{
		return;
	}

	const D3D11_VIEWPORT TargetD3DViewport = MakeD3DViewport(0.f, 0.f, TargetViewport.Rect.GetWidth(), TargetViewport.Rect.GetHeight());

	ID3D11ShaderResourceView* SRVs[] = { ActiveSceneTextures->SceneDepthSRV.Get(), ActiveSceneTextures->GetCurrentSRV() };
	// Fog는 마지막 패스가 아니니 SceneColor에 쓴다
	DrawScreenPass(ActiveSceneTextures->GetTargetRTV(), TargetD3DViewport, SRVs, 2, FName("#Fog"));
	ActiveSceneTextures->SwapPingPong();
}

// ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃ Post Process ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃
// =====================================================================







// =====================================================================
// ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄ Perfomance ⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄⌄

bool FRenderer::InitializeGPUTimerQueries()
{
	constexpr D3D11_QUERY_DESC DisjointDesc{ D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
	constexpr D3D11_QUERY_DESC TimestampDesc{ D3D11_QUERY_TIMESTAMP, 0 };

	for (FGPUTimerQuery& Query : GPUTimerQueries)
	{
		if (FAILED(Device->CreateQuery(&DisjointDesc, &Query.Disjoint)) ||
		    FAILED(Device->CreateQuery(&TimestampDesc, &Query.Start)) ||
		    FAILED(Device->CreateQuery(&TimestampDesc, &Query.End)))
		{
			UE_LOG("GPU Time: 타임스탬프 쿼리 생성에 실패했습니다.");
			return false;
		}
	}

	return true;
}

void FRenderer::BeginGPUTimer()
{
	FGPUTimerQuery& Query = GPUTimerQueries[GPUTimerFrameIndex];

	if (!Query.Disjoint)
	{
		return;
	}

	Context->Begin(Query.Disjoint.Get());
	Context->End(Query.Start.Get());
}

void FRenderer::EndGPUTimer()
{
	FGPUTimerQuery& Query = GPUTimerQueries[GPUTimerFrameIndex];

	// 쿼리가 없어 보고할 수 없더라도 입력 시각은 반드시 비운다.
	// 안 그러면 Trigger()가 계속 튕겨서 Input이 0에 영구 고정된다.
	const int64 InputStartTick = FInputLatencyTimer::Get().ConsumePendingStart();

	if (!Query.Disjoint)
	{
		return;
	}

	Context->End(Query.End.Get());
	Context->End(Query.Disjoint.Get());

	Query.InputStartTick = InputStartTick;
	Query.bPending = true;

	// 다음 프레임은 다음 쿼리 세트를 쓴다.
	GPUTimerFrameIndex = (GPUTimerFrameIndex + 1u) % GPUTimerFrameCount;
}

// GPU 타임스탬프. 결과를 같은 프레임에 바로 읽으면 CPU가 GPU를 기다리게 되므로
// 쿼리 세트를 돌려 쓰고 가장 오래된 것만 회수한다.
void FRenderer::ResolveGPUTimer()
{
	for (uint32 Offset = 0u; Offset < GPUTimerFrameCount; ++Offset)
	{
		const uint32 Index = (GPUTimerFrameIndex + Offset) % GPUTimerFrameCount;
		FGPUTimerQuery& Query = GPUTimerQueries[Index];

		if (!Query.bPending)
		{
			continue;
		}

		// 아직 안 끝났으면 S_FALSE. 기다리지 않고 다음 프레임에 다시 시도한다.
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData{};
		if (Context->GetData(Query.Disjoint.Get(), &DisjointData, sizeof(DisjointData), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
		{
			break;
		}

		Query.bPending = false;

		// Disjoint 쿼리가 끝났다는 건 GPU가 이 프레임 마지막 커맨드를 지났다는 뜻이다.
		// 입력 지연은 클럭 안정성과 무관하므로 Disjoint 여부와 상관없이 보고한다.
		FInputLatencyTimer::Get().Report(Query.InputStartTick);
		Query.InputStartTick = 0;

		// 측정 중 GPU 클럭이 바뀌었다면 시간 값은 믿을 수 없다.
		if (DisjointData.Disjoint || DisjointData.Frequency == 0u)
		{
			continue;
		}

		UINT64 StartTick = 0u;
		UINT64 EndTick = 0u;

		if (Context->GetData(Query.Start.Get(), &StartTick, sizeof(StartTick), 0) != S_OK ||
		    Context->GetData(Query.End.Get(), &EndTick, sizeof(EndTick), 0) != S_OK ||
		    EndTick < StartTick)
		{
			continue;
		}

		LastGPUTimeMs = (EndTick - StartTick) * 1000.0 / static_cast<double>(DisjointData.Frequency);
	}

	// 한 프레임에 세트를 여러 개 회수할 수 있으므로, 누적이 아니라 프레임당 한 번만 넣는다.
	SET_CYCLE_COUNTER("GPU Time", LastGPUTimeMs);
}

// ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃ Perfomamce ⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃⌃
// =====================================================================
