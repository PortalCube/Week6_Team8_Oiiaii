#include <Windows.h>
#include "catch_amalgamated.hpp"
#include "Runtime/Engine/FJson.h"
#include "Runtime/Rendering/FTextRendering.h"

TEST_CASE("Shared text layout preserves centering, spacing and empty text", "[text]")
{
	const FArchive Archive(nlohmann::json::parse(R"({
	    "atlas": {"width": 100, "height": 100},
	    "glyphs": [
	        {"unicode": 65, "advance": 2,
	         "planeBounds": {"left": 0, "right": 1, "top": 1, "bottom": 0},
	         "atlasBounds": {"left": 10, "right": 20, "top": 20, "bottom": 10}},
	        {"unicode": 32, "advance": 3}
	    ]
	})"));
	const FFont Font(Archive);
	TArray<FInstanceData> Instances;
	float Width = 0.0f, Height = 0.0f;
	const FVector4 Color{1.0f, 1.0f, 1.0f, 1.0f};
	TextRendering::BuildGlyphInstances(L"A A", Font, Color, Instances, Width, Height);
	REQUIRE(Instances.size() == 2);
	CHECK(Width == 6.0f);
	CHECK(Height == 1.0f);
	CHECK(Instances[0].World.M[3][1] == -2.5f);
	CHECK(Instances[1].World.M[3][1] == 2.5f);
	CHECK(Instances[0].UVScale.X == Catch::Approx(0.1f));

	// 빈 문자열로 전환할 때 이전 글자 데이터가 남지 않아야 한다.
	TextRendering::BuildGlyphInstances(L"", Font, Color, Instances, Width, Height);
	CHECK(Instances.empty());
	CHECK(Width == 0.0f);
	CHECK(Height == 0.0f);
	TextRendering::BuildGlyphInstances(L"   ", Font, Color, Instances, Width, Height);
	CHECK(Instances.empty());
	CHECK(Width == 0.0f);
}
