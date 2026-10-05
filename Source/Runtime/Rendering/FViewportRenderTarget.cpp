#include "FViewportRenderTarget.h"

// 지정된 크기로 Texture, RTV, SRV를 Device에 생성
bool FViewportRenderTarget::Initialize(ID3D11Device* Device, UINT InWidth, UINT InHeight)
{
	// 기존 리소스를 비움
	Texture.Reset();
	RTV.Reset();
	SRV.Reset();
	Width = 0;
	Height = 0;

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

	// 성공했을 때만 크기를 기록
	Width = InWidth;
	Height = InHeight;

	return true;
}
