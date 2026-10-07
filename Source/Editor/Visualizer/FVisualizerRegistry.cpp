#include "FVisualizerRegistry.h"

#include "Editor/Visualizer/FPrimitiveVisualizer.h"
#include "Editor/Visualizer/FSpotlightVisualizer.h"
#include "Editor/Visualizer/FBillboardVisualizer.h"
#include "Editor/Visualizer/FTextVisualizer.h"

#include "Runtime/Components/UPrimitiveComponent.h"
#include "Runtime/Components/USpotLightComponent.h"
#include "Runtime/Components/UBillboardComponent.h"
#include "Runtime/Components/UAnimatedBillboardComp.h"
#include "Runtime/Components/UTextComponent.h"

FVisualizerRegistry::FVisualizerRegistry()
{
	// 기본 프리미티브 비주얼라이저 등록
	Visualizers.push_back(MakeUnique<FPrimitiveVisualizer>());
	Map[UPrimitiveComponent::StaticClass()] = Visualizers.back().get();

	Visualizers.push_back(MakeUnique<FSpotlightVisualizer>());
	Map[USpotLightComponent::StaticClass()] = Visualizers.back().get();

	Visualizers.push_back(MakeUnique<FBillboardVisualizer>());
	Map[UBillboardComponent::StaticClass()] = Visualizers.back().get();
	Map[UAnimatedBillboardComp::StaticClass()] = Visualizers.back().get();

	Visualizers.push_back(MakeUnique<FTextVisualizer>());
	Map[UTextComponent::StaticClass()] = Visualizers.back().get();
}

IVisualizer* FVisualizerRegistry::FindVisualizer(UClass* ClassType)
{
	while (ClassType != nullptr)
	{
		auto Item = Map.find(ClassType);

		if (Item == Map.end())
		{
			ClassType = ClassType->GetSuperClass();
			continue;
		}

		return Item->second;
	}

	return Visualizers[0].get(); // FPrimitiveVisualizer
}
