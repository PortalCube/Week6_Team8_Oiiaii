#pragma once

struct FLinearColor
{
	float R, G, B, A;

	FLinearColor(float InR = 0.f, float InG = 0.f, float InB = 0.f, float InA = 1.f)
	    : R(InR), G(InG), B(InB), A(InA) {}
};
