#include "FJsonDataWriter.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Utility/WindowsUtil.h"

FJsonDataWriter::FJsonDataWriter()
{
	Direction = EArchiveDirection::Write;
	Clear();
}

void FJsonDataWriter::Clear()
{
	SectionStack = {};
	NextQueue = {};
	ReferenceTable.clear();
	JSON = nullptr;

	// 섹션에 루트 객체 추가
	SectionStack.push(
	    FArchiveSection{
	        .Type = EArchiveSection::Object,
	        .Json = &JSON,
	        .Index = -1,
	    });
}

nlohmann::json& FJsonDataWriter::GetJSON()
{
	return JSON;
}

nlohmann::json FJsonDataWriter::CloneJSON() const
{
	return JSON;
}

void FJsonDataWriter::Serialize(UObject* Object)
{
	Clear();
	JSON = nlohmann::json::array();
	AddReferenceKey(Object);
	while (!NextQueue.empty())
	{
		UObject* Current = NextQueue.front();
		const int32 Reference = ReferenceTable[Current];
		auto& Record = JSON[Reference];
		Record = nlohmann::json::object();
		SectionStack.push({EArchiveSection::Object, &Record, -1});
		UObject* Outer = Current == Object ? nullptr : Current->GetOuter();
		this->Reference("Outer", Outer);
		Current->Serialize(*this);
		SectionStack.pop();
		NextQueue.pop();
	}
}

void FJsonDataWriter::Reference(FStringView Key, UObject*& Value)
{
	// Null 포인터. 그냥 nullptr로 기록
	if (Value == nullptr)
	{
		FieldInternal(Key, nullptr);
		return;
	}

	int32 ReferenceKey = GetReferenceKey(Value);
	if (ReferenceKey == -1)
	{
		ReferenceKey = AddReferenceKey(Value);
	}

	// 등록된 참조키를 등록
	FieldInternal(Key, GetRefJson(ReferenceKey));
}

void FJsonDataWriter::Field(FStringView Key, UObject*& Value)
{
	// JSON에서는 지원 안함
	FieldInternal(Key, nullptr);
}

void FJsonDataWriter::Field(FStringView Key, int32& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, int64& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, uint32& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, uint64& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, float& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, double& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, bool& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, FString& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataWriter::Field(FStringView Key, FWString& Value)
{
	FieldInternal(Key, WindowsUtil::ToString(Value));
}

void FJsonDataWriter::Field(FStringView Key, FVector& Value)
{
	TArray<float> Array;

	for (int i = 0; i < 3; ++i)
	{
		Array.push_back(Value[i]);
	}

	FieldInternal(Key, Array);
}

void FJsonDataWriter::Field(FStringView Key, FVector2& Value)
{
	TArray<float> Array;

	for (int i = 0; i < 2; ++i)
	{
		Array.push_back(Value[i]);
	}

	FieldInternal(Key, Array);
}

void FJsonDataWriter::Field(FStringView Key, FVector4& Value)
{
	TArray<float> Array;

	for (int i = 0; i < 4; ++i)
	{
		Array.push_back(Value[i]);
	}

	FieldInternal(Key, Array);
}

void FJsonDataWriter::BeginSection(FStringView Key)
{
	nlohmann::json& Section = *GetCurrentNode(Key);
	Section = nlohmann::json::object();

	SectionStack.push(
	    FArchiveSection{
	        .Type = EArchiveSection::Object,
	        .Json = &Section,
	        .Index = -1,
	    });
}

void FJsonDataWriter::EndSection()
{
	if (!SectionStack.empty())
	{
		SectionStack.pop();
	}
}

int32 FJsonDataWriter::BeginArray(FStringView Key)
{
	nlohmann::json& Section = *GetCurrentNode(Key);

	Section = nlohmann::json::array();

	SectionStack.push(
	    FArchiveSection{
	        .Type = EArchiveSection::Array,
	        .Json = &Section,
	        .Index = 0,
	    });

	return 0;
}

void FJsonDataWriter::EndArray()
{
	if (!SectionStack.empty())
	{
		SectionStack.pop();
	}
}

nlohmann::json FJsonDataWriter::GetRefJson(int32 ReferenceKey)
{
	return nlohmann::json{ { "$REF", ReferenceKey } };
}

int32 FJsonDataWriter::AddReferenceKey(UObject* Reference)
{
	const int32 Existing = GetReferenceKey(Reference);
	if (Existing >= 0) return Existing;
	const int32 Key = static_cast<int32>(ReferenceTable.size());
	ReferenceTable[Reference] = Key;
	NextQueue.push(Reference);
	return Key;
}

int32 FJsonDataWriter::GetReferenceKey(UObject* Reference)
{
	auto It = ReferenceTable.find(Reference);

	if (It == ReferenceTable.end())
	{
		return -1;
	}

	return It->second;
}

int32 FJsonDataWriter::GetNextArrayIndex()
{
	if (SectionStack.empty())
	{
		return -1;
	}

	if (SectionStack.top().Type != EArchiveSection::Array)
	{
		return -1;
	}

	int32 Result = SectionStack.top().Index;
	++SectionStack.top().Index;

	return Result;
}

nlohmann::json* FJsonDataWriter::CurrentSection()
{
	if (!SectionStack.empty())
	{
		return SectionStack.top().Json;
	}
	else
	{
		return nullptr;
	}
}

nlohmann::json* FJsonDataWriter::GetCurrentNode(FStringView Key)
{
	nlohmann::json& Section = *CurrentSection();

	int32 Index = GetNextArrayIndex();

	if (Index == -1)
	{
		return &Section[Key];
	}
	else
	{
		return &Section[Index];
	}
}
