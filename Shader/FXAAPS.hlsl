Texture2D<float4> SceneTexture : register(t0);
SamplerState LinearSampler : register(s0);

// C++ 상수 버퍼와 순서·크기를 맞춰야 한다.
cbuffer FXAAConstants : register(b6)
{
    float2 InvTextureSize;  // 전체 입력 텍스처의 (1 / Width, 1 / Height).
    float EdgeThreshold;   // 상대적인 경계 판정 기준.
    float EdgeThresholdMin;// 최소 경계 판정 기준.

    float2 UVMin;           // 현재 뷰포트의 샘플 가능한 최소 UV.
    float2 UVMax;           // 현재 뷰포트의 샘플 가능한 최대 UV.

    float Subpixel;        // 서브픽셀 보정 강도.
    float3 Padding;
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
    return SceneTexture.SampleLevel(
        LinearSampler,
        clamp(uv, UVMin, UVMax),
        0);
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
    // 예: 위쪽 픽셀은 ReadColor(uv - dy).

    // TODO 2: 중심과 주변 색을 밝기 값으로 변환하기.

    // TODO 3: 최대 밝기 - 최소 밝기로 대비 계산하기.

    // TODO 4: 대비가 기준보다 작으면 보정 없이 출력하기.

    // TODO 5: 대각선 픽셀도 읽어 경계 방향 판단하기.

    // TODO 6: 경계를 따라 양쪽 끝점 탐색하기.

    // TODO 7: 경계 보정량과 서브픽셀 보정량 계산하기.

    // TODO 8: 샘플 위치를 이동해 보간된 색을 resultColor에 넣기.

    // TODO 9: 입력 SRV와 출력 RTV의 색 공간에 맞춰 최종 색 변환하기.
    // 입력이 sRGB 인코딩된 값이고 출력 RTV가 sRGB라면,
    // 반환 전에 RGB를 선형 값으로 복원해야 한다.

    return resultColor;
}
