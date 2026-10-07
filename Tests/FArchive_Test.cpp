#include "catch_amalgamated.hpp"
#include "Runtime/Serialization/FJsonDataReader.h"
#include "Runtime/Serialization/FJsonDataWriter.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

class UArchiveTestNode : public UObject
{
	DECLARE_UCLASS(UArchiveTestNode, UObject)
public:
	int32 Number = 0;
	UArchiveTestNode* Child = nullptr;
	UArchiveTestNode* Alias = nullptr;
	UArchiveTestNode* Parent = nullptr;
	void Serialize(FArchive& Archive) override
	{
		Super::Serialize(Archive);
		Archive.Field("Number", Number);
		Archive.Reference("RootComponent", Child);
		Archive.Reference("Alias", Alias);
		Archive.Reference("Parent", Parent);
	}
};
IMPLEMENT_UCLASS(UArchiveTestNode, UObject)

class UArchiveTestOwner : public UArchiveTestNode
{
	DECLARE_UCLASS(UArchiveTestOwner, UArchiveTestNode)
public:
	void Initialize() override { Child = NewObject<UArchiveTestNode>(this); }
	void Release() override { DestroyObject(Child); }
};
IMPLEMENT_UCLASS(UArchiveTestOwner, UArchiveTestNode)

TEST_CASE("JSON archive fields and nested arrays round trip", "[unit][archive]")
{
	auto SerializeFields = [](FArchive& Archive, int64& Signed, uint64& Unsigned,
							 FWString& Text, FVector4& Vector, TArray<int32>& Values)
	{
		Archive.Field("Signed", Signed);
		Archive.Field("Unsigned", Unsigned);
		Archive.Field("Text", Text);
		Archive.Field("Vector", Vector);
		Archive.BeginSection("Nested");
		int32 Count = Archive.BeginArray("Values");
		if (Archive.IsReading()) Values.resize(Count);
		else Count = static_cast<int32>(Values.size());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Archive.BeginSection("");
			Archive.Field("Value", Values[Index]);
			Archive.EndSection();
		}
		Archive.EndArray();
		Archive.EndSection();
	};
	FJsonDataWriter Writer;
	int64 Signed = std::numeric_limits<int64>::min();
	uint64 Unsigned = std::numeric_limits<uint64>::max();
	FWString Text = L"정글 🎮";
	FVector4 Vector{1, 2, 3, 4};
	TArray<int32> Values{-1, 0, 2147483647};
	SerializeFields(Writer, Signed, Unsigned, Text, Vector, Values);
	FJsonDataReader Reader(Writer.CloneJSON());
	int64 LoadedSigned = 0;
	uint64 LoadedUnsigned = 0;
	FWString LoadedText;
	FVector4 LoadedVector;
	TArray<int32> LoadedValues;
	SerializeFields(Reader, LoadedSigned, LoadedUnsigned, LoadedText, LoadedVector, LoadedValues);
	CHECK(LoadedSigned == Signed);
	CHECK(LoadedUnsigned == Unsigned);
	CHECK(LoadedText == Text);
	CHECK(LoadedVector == Vector);
	CHECK(LoadedValues == Values);
}

TEST_CASE("Graph preserves defaults, Outer zero, aliases, cycles and reuse", "[unit][archive]")
{
	auto* Source = NewObject<UArchiveTestOwner>(nullptr);
	Source->Initialize();
	Source->Number = 42;
	Source->Child->Number = 7;
	Source->Alias = Source->Child;
	Source->Child->Parent = Source;
	auto* Target = NewObject<UArchiveTestOwner>(nullptr);
	Target->Initialize();
	auto* DefaultChild = Target->Child;
	FJsonDataWriter Writer;
	Writer.Serialize(Source);
	REQUIRE(Writer.GetJSON().size() == 2);
	FJsonDataReader Reader(Writer.CloneJSON());
	Reader.Serialize(Target);
	CHECK(Target->Number == 42);
	CHECK(Target->Child == DefaultChild);
	CHECK(Target->Child->Number == 7);
	CHECK(Target->Child->GetOuter() == Target);
	CHECK(Target->Alias == Target->Child);
	CHECK(Target->Child->Parent == Target);
	CHECK(Target->Parent == nullptr);
	Source->Child->Number = 99;
	Writer.Serialize(Source);
	REQUIRE(Writer.GetJSON().size() == 2);
	Reader.SetJSON(Writer.CloneJSON());
	Reader.Serialize(Target);
	CHECK(Target->Child->Number == 99);
	DestroyObject(Target);
	DestroyObject(Source);
}

TEST_CASE("Reader creates referenced objects with Outer index zero", "[unit][archive]")
{
	auto* Source = NewObject<UArchiveTestNode>(nullptr);
	Source->Child = NewObject<UArchiveTestNode>(Source);
	Source->Child->Number = 23;
	auto* Target = NewObject<UArchiveTestNode>(nullptr);
	FJsonDataWriter Writer;
	Writer.Serialize(Source);
	FJsonDataReader Reader(Writer.CloneJSON());
	Reader.Serialize(Target);
	REQUIRE(Target->Child != nullptr);
	CHECK(Target->Child->GetOuter() == Target);
	CHECK(Target->Child->Number == 23);
	DestroyObject(Target->Child);
	DestroyObject(Source->Child);
	DestroyObject(Target);
	DestroyObject(Source);
}

TEST_CASE("Missing optional JSON fields preserve defaults", "[unit][archive]")
{
	FJsonDataReader Reader(nlohmann::json::object());
	int32 Default = 17;
	Reader.Field("Missing", Default);
	CHECK(Default == 17);
}

TEST_CASE("Clear resets partial traversal and writer output", "[unit][archive]")
{
	FJsonDataWriter Writer;
	int32 Value = 7;
	Writer.BeginSection("OldSection");
	Writer.Field("Value", Value);
	Writer.Clear();
	CHECK(Writer.GetJSON().is_null());
	Writer.Field("Value", Value);
	CHECK(Writer.GetJSON().size() == 1);
	CHECK(Writer.GetJSON().at("Value") == Value);
	CHECK(Writer.IsWriting());

	FJsonDataReader Reader(nlohmann::json{{"Values", {10, 20}}});
	CHECK(Reader.BeginArray("Values") == 2);
	Reader.Field("", Value);
	CHECK(Value == 10);
	Reader.Clear();
	CHECK(Reader.BeginArray("Values") == 2);
	Reader.Field("", Value);
	CHECK(Value == 10);
	Reader.Field("", Value);
	CHECK(Value == 20);
	Reader.EndArray();
	CHECK(Reader.IsReading());
}
