Texture2D<float4> SceneTexture : register(t0);
SamplerState LinearSampler : register(s0);

// 고정된 경계 판정 기준을 사용함.
static const float EdgeThreshold = 0.166;
static const float EdgeThresholdMin = 0.0833;

cbuffer FXAAConstants : register(b6)
{
    float2 InvTextureSize;  // 1Pixel의 UV 크기.

    float2 UVMin;           // 현재 뷰포트의 샘플 가능한 최소 UV.
    float2 UVMax;           // 현재 뷰포트의 샘플 가능한 최대 UV.

    float Subpixel;        // 서브픽셀 보정 강도.
    float Padding;         // 상수 버퍼를 16바이트 단위로 맞춤. 전체 크기 32바이트.
};

// 기존 ScreenQuadVS의 출력과 대응한다.
struct PS_IN
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEXCOORD0;
};

// 분할 화면 바깥의 색이 섞이지 않도록 샘플 범위를 제한한다.
float4 ReadColor(float2 uv)
{
    return SceneTexture.SampleLevel(LinearSampler, clamp(uv, UVMin, UVMax), 0);
}

float Luma(float3 color)
{
	return dot(color, float3(0.299, 0.587, 0.114));
}

// 한 방향으로 최대 12픽셀 탐색하며, 실제 샘플 위치를 거리 계산에 사용함.
void SearchEdge(
    float2 startUV, float2 stepUV, float edgeLuma, float searchThreshold,
    out float2 endUV, out float endDelta, out bool found)
{
    float2 searchStart = clamp(startUV, UVMin, UVMax);
    endUV = searchStart;
    endDelta = Luma(ReadColor(endUV).rgb) - edgeLuma;
    found = false;

	// 최대 탐색 픽셀 제한
    const int MaxSearchSteps = 12;
    [loop]
    for (int step = 1; step <= MaxSearchSteps; ++step)
    {
        float2 candidateUV = searchStart + stepUV * float(step);
        float2 sampleUV = clamp(candidateUV, UVMin, UVMax);

        // 뷰포트 끝에서 더 이동할 수 없으면 탐색 종료함.
        if (all(sampleUV == endUV))
        {
            break;
        }

        endUV = sampleUV;
        endDelta = Luma(ReadColor(endUV).rgb) - edgeLuma;

        // 밝기 변화 조건을 만족한 경우에만 끝점으로 인정함.
        if (abs(endDelta) >= searchThreshold)
        {
            found = true;
            break;
        }

        // 범위를 벗어나면 마지막 유효 위치까지만 샘플하고 종료함.
        if (any(candidateUV != sampleUV))
        {
            break;
        }
    }
}

float4 MainPS(PS_IN input) : SV_Target
{
    // 공유 텍스처의 전역 픽셀 좌표를 UV로 변환한다.
    float2 uv = input.Pos.xy * InvTextureSize;

    // 텍스처에서 가로·세로 1픽셀 이동하는 거리.
    float2 dx = float2(InvTextureSize.x, 0.0);
    float2 dy = float2(0.0, InvTextureSize.y);

    float4 centerColor = ReadColor(uv);
    float4 resultColor = centerColor;

    // TODO 1: 상하좌우 픽셀의 색을 읽기.
	float4 eastColor = ReadColor(uv + dx);
	float4 westColor = ReadColor(uv - dx);
	float4 southColor = ReadColor(uv + dy);
	float4 northColor = ReadColor(uv - dy);

    // TODO 2: 중심과 주변 색을 밝기 값으로 변환하기.
	float lumaM = Luma(centerColor.rgb);
	float lumaE = Luma(eastColor.rgb);
	float lumaW = Luma(westColor.rgb);
	float lumaS = Luma(southColor.rgb);
	float lumaN = Luma(northColor.rgb);

	// TODO 3: 최대 밝기 - 최소 밝기로 대비 계산하기.
	float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
	float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));

	float contrast = lumaMax - lumaMin;

    // TODO 4: 대비가 기준보다 작으면 보정 없이 출력하기.
	float threshold = max(EdgeThresholdMin, lumaMax * EdgeThreshold);
	if (contrast < threshold)
	{
		return centerColor;
	}

    // TODO 5: 대각선 픽셀도 읽어 경계 방향 판단하기.
	float lumaNW = Luma(ReadColor(uv - dx - dy).rgb); // 좌상
	float lumaNE = Luma(ReadColor(uv + dx - dy).rgb); // 우상
	float lumaSW = Luma(ReadColor(uv - dx + dy).rgb); // 좌하
	float lumaSE = Luma(ReadColor(uv + dx + dy).rgb); // 우하

	float edgeHorizontal =
    abs(lumaNW - 2.0 * lumaW + lumaSW) +
    2.0 * abs(lumaN - 2.0 * lumaM + lumaS) +
    abs(lumaNE - 2.0 * lumaE + lumaSE);

	float edgeVertical =
    abs(lumaNW - 2.0 * lumaN + lumaNE) +
    2.0 * abs(lumaW - 2.0 * lumaM + lumaE) +
    abs(lumaSW - 2.0 * lumaS + lumaSE);

	bool isHorizontal = edgeHorizontal >= edgeVertical;
	
    // TODO 6: 경계를 따라 양쪽 끝점 탐색하기.
	float lumaNegative = isHorizontal ? lumaN : lumaW;
	float lumaPositive = isHorizontal ? lumaS : lumaE;

	float gradientNegative = abs(lumaNegative - lumaM);
	float gradientPositive = abs(lumaPositive - lumaM);

	bool useNegative = gradientNegative >= gradientPositive;

	float2 normal = isHorizontal ? dy : dx;
	normal *= useNegative ? -1.0 : 1.0;
	float2 edgeUV = uv + normal * 0.5;

	float edgeLuma = 0.5 * (lumaM + (useNegative ? lumaNegative : lumaPositive));

	// 가로 경계면 좌우로, 세로 경계면 위아래로 탐색.
	float2 tangent = isHorizontal ? dx : dy;

	// 선택한 경계의 밝기 변화량을 기준으로 탐색 종료 조건 설정.
	float searchThreshold = 0.25 * max(gradientNegative, gradientPositive);
	float2 negativeUV, positiveUV;
	float negativeDelta, positiveDelta;
	bool negativeDone, positiveDone;

	SearchEdge(edgeUV, -tangent, edgeLuma, searchThreshold,
		negativeUV, negativeDelta, negativeDone);
	SearchEdge(edgeUV, tangent, edgeLuma, searchThreshold,
		positiveUV, positiveDelta, positiveDone);

    // TODO 7: 경계 보정량과 서브픽셀 보정량 계산하기.

	float distanceNegative = isHorizontal ? uv.x - negativeUV.x : uv.y - negativeUV.y;
	float distancePositive = isHorizontal ? positiveUV.x - uv.x : positiveUV.y - uv.y;

	float spanLength = distanceNegative + distancePositive;
	float nearestDistance = min(distanceNegative, distancePositive);

	bool nearerNegative = distanceNegative < distancePositive;
	float nearestDelta = nearerNegative ? negativeDelta : positiveDelta;

	// ── 2. 경계 보정량 ──

	
	bool nearestFound = nearerNegative ? negativeDone : positiveDone;

	// 중심이 양쪽 끝점의 중간이면 0.
	// 한쪽 끝점에 가까울수록 0.5에 가까워진다.
	float edgeOffset = 0.5 - nearestDistance / max(spanLength, 1e-6);

	// 중심과 가까운 끝점이 경계 기준 밝기의 서로 반대편에 있는지 확인.
	// 같은 편이면 이 경계 보정을 적용하지 않는다.
	//bool validEdge = (nearestDelta < 0.0) != ((lumaM - edgeLuma) < 0.0);
	bool validEdge = nearestFound && ((nearestDelta < 0.0) != ((lumaM - edgeLuma) < 0.0));
	edgeOffset = validEdge ? edgeOffset : 0.0;


	// ── 3. 서브픽셀 보정량 ──

	// 상하좌우는 가중치 2, 대각선은 가중치 1.
	// 가중치 합이 12이므로 12로 나눈다.
	float averageLuma =
    (2.0 * (lumaN + lumaS + lumaE + lumaW)
     + lumaNW + lumaNE + lumaSW + lumaSE) / 12.0;

	// 중심만 주변 평균과 크게 다른 정도를 대비로 정규화한다.
	float subpixelRatio = saturate(abs(averageLuma - lumaM) / max(contrast, 1e-6));

	// 보정 강도가 부드럽게 변하도록 곡선을 적용한다.
	float subpixelCurve = subpixelRatio * subpixelRatio * (3.0 - 2.0 * subpixelRatio);
	// 서브픽셀 보정 강도를 0~1로 제한함.
	float subpixelOffset = subpixelCurve * subpixelCurve * saturate(Subpixel);


	// ── 4. 최종 보정량 ──

	// 두 보정량을 더하지 않고 더 큰 값을 선택한다.
	float finalOffset = max(edgeOffset, subpixelOffset);

	
    // TODO 8: 샘플 위치를 이동해 보간된 색을 resultColor에 넣기.

	// 원본 픽셀 중앙에서 경계에 수직으로 이동.
	float2 finalUV = uv + normal * finalOffset;

	// 이동한 위치에서 보간된 색을 읽고, 원본 알파 유지.
	resultColor = float4(ReadColor(finalUV).rgb, centerColor.a);

	return resultColor;
}
