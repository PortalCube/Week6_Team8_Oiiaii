#pragma once

#include "Runtime/Engine/Types/IntTypes.h"
#include "ThirdParty/Json/json_fwd.hpp"

/*
* 이 파일은 Engine 이곳저곳에서 쓰이는 enum이나 여러 type을 정의합니다.
* 특정 타입이 어느 한 클래스 종속적이더라도 가급적 여기에 선언하세요!
*/

using Json = nlohmann::json;

////////////////////////////////////////////////////////////
// UObject
////////////////////////////////////////////////////////////

template <typename T>
concept UObjectType = std::derived_from<T, class UObject>;



////////////////////////////////////////////////////////////
// AActor
////////////////////////////////////////////////////////////

template <typename T>
concept AActorType = std::derived_from<T, class AActor>;



////////////////////////////////////////////////////////////
// FArchive
////////////////////////////////////////////////////////////

enum EArchiveDirection : uint8
{
	None,
	Write,
	Read,
};

enum EArchiveMode : uint8
{
	Persistence,
	Duplicate,
};

enum EArchiveSection : uint8
{
	Object,
	Array,
};

struct FArchiveSection
{
	EArchiveSection Type = EArchiveSection::Object;
	nlohmann::json* Json = nullptr;
	int32 Index = 0;
};



////////////////////////////////////////////////////////////
// FWorldContext
////////////////////////////////////////////////////////////

enum class EWorldType: uint8
{
	None = 0,
	Game,
	Editor,
	PIE,
	EditorPreview
};
