#pragma once

#include <d3d11.h>
#include <wrl/client.h>

// Screen Pass에 쓰이는 Color와 Depth Texture
struct FSceneTextures
{
	// 현재 SceneColorTexture와 SceneDepthTexture의 크기
	UINT Width, Height;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneColorTexture;		// Draw시 이 Texture에 그려진다
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SceneColorRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneColorSRV;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneDepthTexture;		// 이 Texture에는 깊이 정보가 그려진다
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> SceneDepthDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneDepthSRV;   // Depth 읽는 SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneStencilSRV; // 스텐실 읽는 SRV

	bool InitializeSceneTextures(ID3D11Device* Device, UINT Width, UINT Height)
	{
		D3D11_TEXTURE2D_DESC SceneDeptTexturehDesc = {
			.Width = Width,
			.Height = Height,
			.MipLevels = 1u,
			.ArraySize = 1u,
			.Format = DXGI_FORMAT_R24G8_TYPELESS,
			.SampleDesc = {
			    .Count = 1u,
			},
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
		};

		// SceneDepthTexture 생성
		HRESULT Result = Device->CreateTexture2D(&SceneDeptTexturehDesc, nullptr, &SceneDepthTexture);
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

		// SceneColorTexture 생성
		D3D11_TEXTURE2D_DESC SceneColorTextureDesc{
			.Width = Width,
			.Height = Height,
			.MipLevels = 1u,
			.ArraySize = 1u,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = { .Count = 1u },
			.Usage = D3D11_USAGE_DEFAULT,
			.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		};

		Result = Device->CreateTexture2D(&SceneColorTextureDesc, nullptr, &SceneColorTexture);
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

		// SceneColorSRV 생성
		Result = Device->CreateShaderResourceView(SceneColorTexture.Get(), nullptr, &SceneColorSRV);
		if (FAILED(Result))
		{
			return false;
		}

		return true;
	}

		void Reset()
	{
		SceneColorTexture.Reset();
		SceneColorRTV.Reset();
		SceneColorSRV.Reset();

		SceneDepthTexture.Reset();
		SceneDepthSRV.Reset();
		SceneDepthDSV.Reset();
		SceneStencilSRV.Reset();
	}
};

struct FPairHash
{
	size_t operator()(const std::pair<UINT, UINT>& Key) const
	{
		return std::hash<uint64>{}(Key.first) ^ (std::hash<uint64>{}(Key.second) << 1);
	}
};
