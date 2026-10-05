#pragma once

#include <d3d11.h>
#include <wrl/client.h>

struct FViewportRenderTarget
{
public:
	friend class FRenderer;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> GetTexture() { return Texture; }
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> GetRTV() { return RTV; }
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV() { return SRV; }

	// 지정된 크기로 Texture, RTV, SRV를 Device에 생성
	bool Initialize(ID3D11Device* Device, UINT Width, UINT Height)
	{
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

		HRESULT Result = Device->CreateTexture2D(&SceneColorTextureDesc, nullptr, &Texture);
		if (FAILED(Result))
		{
			return false;
		}

		// SceneColorRTV 생성
		Result = Device->CreateRenderTargetView(Texture.Get(), nullptr, &RTV);
		if (FAILED(Result))
		{
			return false;
		}

		// SceneColorSRV 생성
		Result = Device->CreateShaderResourceView(Texture.Get(), nullptr, &SRV);
		if (FAILED(Result))
		{
			return false;
		}

		return true;
	}

private:
	FViewportRenderTarget() = default;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
};
