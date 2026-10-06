#include "FViewport.h"
#include "Runtime/Rendering/FViewportRenderTarget.h"

// 아래 함수들은 TSharedPtr<FViewportRenderTarget>의 소멸자/대입 연산자를 생성하므로
// FViewportRenderTarget의 완전한 정의가 보이는 이 파일에서만 정의한다.
FViewport::FViewport() = default;
FViewport::~FViewport() = default;
FViewport::FViewport(const FViewport& Other) = default;
FViewport& FViewport::operator=(const FViewport& Other) = default;
FViewport::FViewport(FViewport&& Other) noexcept = default;
FViewport& FViewport::operator=(FViewport&& Other) noexcept = default;
