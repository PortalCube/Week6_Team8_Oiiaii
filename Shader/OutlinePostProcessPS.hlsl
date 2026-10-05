// 외곽선 후처리 픽셀 셰이더
Texture2D<float4> SceneTexture : register(t0);
// 2컴포넌트 포맷(X24_G8)에 맞게 uint2로 선언
Texture2D<uint2> StencilTexture : register(t1);

struct PSIn
{
	float4 Pos : SV_POSITION;
	float2 UV : TEXCOORD0;
};

// 스텐실 값을 안전하게 읽는 함수 (x든 y든 1이 있으면 검출)
uint SampleStencil(int2 Coord)
{
	uint2 Stencil = StencilTexture.Load(int3(Coord, 0));
	return max(Stencil.x, Stencil.y);
}

float4 MainPS(PSIn Input) : SV_Target
{
	const int2 PixelCoord = int2(Input.Pos.xy);
	uint Width, Height;
	StencilTexture.GetDimensions(Width, Height);

    // 현재 픽셀의 스텐실 값 확인
	const uint CurrentStencil = SampleStencil(PixelCoord);
	const float4 SceneColor = SceneTexture.Load(int3(PixelCoord, 0));

    // 본체 영역은 원래 색상 유지
	if (CurrentStencil == 1)
	{
		return SceneColor;
	}

    // 8방향 주변 스텐실 검사
	const int Thickness = 2;
	const int2 Offsets[8] =
	{
		int2(Thickness, 0), int2(-Thickness, 0),
        int2(0, Thickness), int2(0, -Thickness),
        int2(Thickness, Thickness), int2(-Thickness, Thickness),
        int2(Thickness, -Thickness), int2(-Thickness, -Thickness)
	};

	uint NeighborStencil = 0;
	for (int i = 0; i < 8; ++i)
	{
		const int2 Neighbor = PixelCoord + Offsets[i];
		if (Neighbor.x >= 0 && Neighbor.y >= 0 &&
            Neighbor.x < int(Width) && Neighbor.y < int(Height))
		{
			NeighborStencil |= SampleStencil(Neighbor);
		}
	}

    // 주변에 본체가 닿아 있으면 주황색 외곽선 출력
	if (NeighborStencil > 0)
	{
		return float4(1.0f, 0.55f, 0.0f, 1.0f);
	}

	return SceneColor;
}
