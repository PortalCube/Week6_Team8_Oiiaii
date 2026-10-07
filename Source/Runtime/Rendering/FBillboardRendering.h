#pragma once

#include "Runtime/Engine/FCamera.h"
#include "Runtime/Geometry/FTransform.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector4.h"

namespace BillboardRendering
{
	inline FMatrix MakeBillboardMatrix(const FTransform& Transform, const FCamera& Camera)
	{
		FMatrix CameraRotation = Camera.GetRotationMatrix();
		FVector ViewForward =
		    CameraRotation.TransformPointRow(FVector{ 1.0f, 0.0f, 0.0f }, 0.0f); // X+
		FVector ViewRight =
		    CameraRotation.TransformPointRow(FVector{ 0.0f, 1.0f, 0.0f }, 0.0f); // Y+
		FVector ViewUp =
		    CameraRotation.TransformPointRow(FVector{ 0.0f, 0.0f, 1.0f }, 0.0f); // Z+

		FVector Up = ViewUp * Transform.GetScale3D().Z;
		FVector Right = ViewRight * Transform.GetScale3D().Y;

		return FMatrix{
			FVector4{ ViewForward, 0.0f },
			FVector4{ Right, 0.0f },
			FVector4{ Up, 0.0f },
			FVector4{ Transform.GetLocation(), 1.0f },
		};
	}
} // namespace BillboardRendering
