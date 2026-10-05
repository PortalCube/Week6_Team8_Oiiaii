#pragma once

#include <d3d11.h>
#include <wrl/client.h>

struct FViewportRenderTarget
{
public:
	friend class FRenderer;

	//ID3D11Texture2D* GetTexture() const { return Texture.Get(); }
	ID3D11RenderTargetView* GetRTV() const { return RTV.Get(); }
	ID3D11ShaderResourceView* GetSRV() const { return SRV.Get(); }

	// 생성된 텍스처의 크기. 뷰포트 크기와 비교해 재생성 여부를 판단할 때 사용
	UINT GetWidth() const { return Width; }
	UINT GetHeight() const { return Height; }

	// 지정된 크기로 Texture, RTV, SRV를 Device에 생성
	bool Initialize(ID3D11Device* Device, UINT InWidth, UINT InHeight);

private:
	FViewportRenderTarget() = default;

	//Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;

	UINT Width = 0;
	UINT Height = 0;
};
