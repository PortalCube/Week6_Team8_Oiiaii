#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <functional>
#include <utility>
#include "Runtime/Engine/Types/IntTypes.h"

// 뷰포트 하나를 그리는 동안 쓰이는 텍스처. 크기는 그 뷰포트와 같아야 한다.
// (출력 RT와 SceneDepthDSV를 함께 바인딩하므로 크기가 다르면 OMSetRenderTargets가 실패한다)
struct FSceneTextures
{
	// 현재 SceneColorTexture와 SceneDepthTexture의 크기
	UINT Width = 0;
	UINT Height = 0;

	// 마지막으로 사용된 프레임 카운트. 오래 안 쓰이면 정리
	uint64 LastUsedFrame = 0;

private:

	// Post Process로 SceneColor를 읽어야 하는데 같은 텍스쳐에 RTV(쓰기)와 SRV(읽기)를 동시에 바인딩 할 수 없다.
	// 따라서 SceneColor 텍스쳐를 2개 두고 번갈아 가며 읽는다 (하나는 출력 RTV, 하나는 입력 SRV) (ping-pong)

	// Taerget : RTV가 그리는(쓰는) 쪽.  /  반대는 Current : SRV가 읽는 쪽.
	// Ping 이 True 이면 Ping이 Current이 되고, False 이면 Ping이 Target이 된다
	bool Ping = true;

	// Ping
	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneColorTexture;		    // Draw시 이 Texture에 그려진다
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SceneColorRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneColorSRV;
	// Pong
	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneColorTexture2;	
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> SceneColorRTV2;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneColorSRV2;

public:

	// Depth Stencil
	Microsoft::WRL::ComPtr<ID3D11Texture2D> SceneDepthTexture;			// 이 Texture에는 깊이 정보가 그려진다
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> SceneDepthDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneDepthSRV;		// Depth 읽는 SRV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SceneStencilSRV;	// stencil 읽는 SRV

	// 받은 크기대로 SceneTextures를 생성
	bool InitializeSceneTextures(ID3D11Device* Device, UINT InWidth, UINT InHeight);
	void Reset();

	void SwapPingPong() { Ping ? Ping = false : Ping = true; }
	ID3D11ShaderResourceView* GetCurrentSRV() const { return Ping ? SceneColorSRV.Get() : SceneColorSRV2.Get(); }
	ID3D11RenderTargetView* GetCurrentRTV() const { return Ping ? SceneColorRTV.Get() : SceneColorRTV2.Get(); }
	ID3D11RenderTargetView* GetTargetRTV() const { return !Ping ? SceneColorRTV.Get() : SceneColorRTV2.Get(); }
};

// FRenderer의 SceneTexturesPool(TMap) 용 hash
struct FPairHash
{
	size_t operator()(const std::pair<UINT, UINT>& Key) const
	{
		const uint64 Combined = (static_cast<uint64>(Key.first) << 32) | static_cast<uint64>(Key.second);
		return std::hash<uint64>{}(Combined);
	}
};
