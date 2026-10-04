#pragma once
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/Rendering/FMesh.h"
#include "Runtime/Rendering/FMaterial.h"

class FCamera;

class FGrid
{
private:
	float CellSize = 1.0f;
	bool bVisible = true;

public:
	void DrawLine(FRenderer& Renderer, const FCamera& Camera);

	float GetCellSize() const { return CellSize; }
	void SetCellSize(float InCellSize) { CellSize = (InCellSize > 0.01f) ? InCellSize : 0.01f; }
};