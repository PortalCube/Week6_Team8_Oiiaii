#include "FSceneTextures.h"

// 받은 크기대로 SceneTextures를 생성한다
bool FSceneTextures::InitializeSceneTextures(ID3D11Device* Device, UINT InWidth, UINT InHeight)
{
	// 기존 리소스를 먼저 비움
	Reset();

	if (!Device || InWidth == 0 || InHeight == 0)
	{
		return false;
	}

	// SceneColorTexture 생성
	D3D11_TEXTURE2D_DESC SceneColorTextureDesc{
		.Width = InWidth,
		.Height = InHeight,
		.MipLevels = 1u,
		.ArraySize = 1u,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = { .Count = 1u },
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
	};
	HRESULT Result = Device->CreateTexture2D(&SceneColorTextureDesc, nullptr, &SceneColorTexture);
	if (FAILED(Result))
	{
		return false;
	}
	Result = Device->CreateTexture2D(&SceneColorTextureDesc, nullptr, &SceneColorTexture2);
	if (FAILED(Result))
	{
		return false;
	}

	// SceneColorRTV 생성
	Result = Device->CreateRenderTargetView(SceneColorTexture.Get(), nullptr, &SceneColorRTV);
	if (FAILED(Result))
	{
		return false;
	}
	Result = Device->CreateRenderTargetView(SceneColorTexture2.Get(), nullptr, &SceneColorRTV2);
	if (FAILED(Result))
	{
		return false;
	}

	// SceneColorSRV 생성
	Result = Device->CreateShaderResourceView(SceneColorTexture.Get(), nullptr, &SceneColorSRV);
	if (FAILED(Result))
	{
		return false;
	}
	Result = Device->CreateShaderResourceView(SceneColorTexture2.Get(), nullptr, &SceneColorSRV2);
	if (FAILED(Result))
	{
		return false;
	}

	// SceneDepthTexture 생성
	D3D11_TEXTURE2D_DESC SceneDeptTexturehDesc = {
		.Width = InWidth,
		.Height = InHeight,
		.MipLevels = 1u,
		.ArraySize = 1u,
		.Format = DXGI_FORMAT_R24G8_TYPELESS,
		.SampleDesc = {
		    .Count = 1u,
		},
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
	};
	Result = Device->CreateTexture2D(&SceneDeptTexturehDesc, nullptr, &SceneDepthTexture);
	if (FAILED(Result))
	{
		return false;
	}

	// SceneDepthDSV 생성
	D3D11_DEPTH_STENCIL_VIEW_DESC SceneDepthDSVDesc = {
		.Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
		.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
	};
	Result = Device->CreateDepthStencilView(SceneDepthTexture.Get(), &SceneDepthDSVDesc, &SceneDepthDSV);
	if (FAILED(Result))
	{
		return false;
	}

	// SceneDepthSRV 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC SceneDepthSRVDesc = {
		.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS, // DepthSRV의 포멧은 TYPELESS이어야함
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = { .MostDetailedMip = 0, .MipLevels = 1 },
	};
	Result = Device->CreateShaderResourceView(SceneDepthTexture.Get(), &SceneDepthSRVDesc, &SceneDepthSRV);
	if (FAILED(Result))
	{
		return false;
	}

	// SceneStencilSRV 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC SceneStencilSRVDesc = {
		.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT,
		.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		.Texture2D = { .MostDetailedMip = 0, .MipLevels = 1 },
	};
	Result = Device->CreateShaderResourceView(SceneDepthTexture.Get(), &SceneStencilSRVDesc, &SceneStencilSRV);
	if (FAILED(Result))
	{
		return false;
	}

	// 모든 리소스 생성에 성공했을 때만 크기를 기록
	Width = InWidth;
	Height = InHeight;

	return true;
}

void FSceneTextures::Reset()
{
	SceneColorTexture.Reset();
	SceneColorRTV.Reset();
	SceneColorSRV.Reset();
	SceneColorTexture2.Reset();
	SceneColorRTV2.Reset();
	SceneColorSRV2.Reset();

	SceneDepthTexture.Reset();
	SceneDepthSRV.Reset();
	SceneDepthDSV.Reset();
	SceneStencilSRV.Reset();

	Width = 0;
	Height = 0;

	Ping = true;
}
