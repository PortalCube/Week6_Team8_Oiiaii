#include "FJsonDataReader.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Utility/WindowsUtil.h"

FJsonDataReader::FJsonDataReader(const nlohmann::json& InJSON)
{
	Direction = EArchiveDirection::Read;
	SetJSON(InJSON);
}

void FJsonDataReader::SetJSON(const nlohmann::json& InJSON)
{
	JSON = InJSON;
	Clear();
}

void FJsonDataReader::Clear()
{
	SectionStack = {};
	NextQueue = {};
	ReferenceTable.clear();
	SectionStack.push({EArchiveSection::Object, &JSON, -1});
}

void FJsonDataReader::Serialize(UObject* Object)
{
	Clear();
	
	// 새로 복원해야할 객체.
	ReferenceTable[0] = Object;
	NextQueue.push(0);

	while (!NextQueue.empty())
	{
		const int32 Current = NextQueue.front();

		SectionStack.push(
		    FArchiveSection{
		        .Type = EArchiveSection::Object,
		        .Json = &JSON[Current],
		        .Index = -1,
		    });

		ReferenceTable[Current]->Serialize(*this);

		SectionStack.pop();
		NextQueue.pop();
	}
}

void FJsonDataReader::Reference(FStringView Key, UObject*& Value)
{
	const nlohmann::json* Node = GetCurrentNode(Key);

	if (!Node)
	{
		return;
	}

	const int32 Reference = GetRefKey(*Node);

	if (Reference < 0)
	{
		Value = nullptr;
	}
	else
	{
		Value = AddReferenceKey(Reference, Value);
	}
}

void FJsonDataReader::Field(FStringView Key, UObject*& Value)
{
	// JSON에서는 지원 안함
	GetCurrentNode(Key);
	Value = nullptr;
}

void FJsonDataReader::Field(FStringView Key, int32& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, int64& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, uint32& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, uint64& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, float& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, double& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, bool& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, FString& Value)
{
	FieldInternal(Key, Value);
}

void FJsonDataReader::Field(FStringView Key, FWString& Value)
{
	FString UTF8Value = WindowsUtil::ToString(Value);
	FieldInternal(Key, UTF8Value);
	Value = WindowsUtil::ToWString(UTF8Value);
}

void FJsonDataReader::Field(FStringView Key, FVector& Value)
{
	TArray<float> Array{};

	FieldInternal(Key, Array);

	for (int i = 0; i < 3; ++i)
	{
		Value[i] = Array[i];
	}
}

void FJsonDataReader::Field(FStringView Key, FVector2& Value)
{
	TArray<float> Array{};

	FieldInternal(Key, Array);

	for (int i = 0; i < 2; ++i)
	{
		Value[i] = Array[i];
	}
}

void FJsonDataReader::Field(FStringView Key, FVector4& Value)
{
	TArray<float> Array{};

	FieldInternal(Key, Array);

	for (int i = 0; i < 4; ++i)
	{
		Value[i] = Array[i];
	}
}

void FJsonDataReader::BeginSection(FStringView Key)
{
	nlohmann::json* Node = GetCurrentNode(Key);

	SectionStack.push(
	    FArchiveSection{
	        .Type = EArchiveSection::Object,
	        .Json = Node,
	        .Index = -1,
	    });
}

void FJsonDataReader::EndSection()
{
	if (!SectionStack.empty())
	{
		SectionStack.pop();
	}
}

int32 FJsonDataReader::BeginArray(FStringView Key)
{
	nlohmann::json* Node = GetCurrentNode(Key);

	SectionStack.push(
	    FArchiveSection{
	        .Type = EArchiveSection::Array,
	        .Json = Node,
	        .Index = 0,
	    });

	return Node->size();
}

void FJsonDataReader::EndArray()
{
	if (!SectionStack.empty())
	{
		SectionStack.pop();
	}
}

int32 FJsonDataReader::GetRefKey(const nlohmann::json& Node)
{
	if (Node == nullptr)
	{
		return -1;
	}

	return Node["$REF"].get<int32>();
}

UObject* FJsonDataReader::AddReferenceKey(int32 Reference, UObject* Existing)
{
	UObject* Object = GetReferenceKey(Reference);

	if (Object)
	{
		return Object;
	}

	const nlohmann::json& Target = JSON[Reference];

	// 객체의 Outer 찾기
	const int32 OuterKey = GetRefKey(Target["Outer"]);
	UObject* Outer = nullptr;
	if (OuterKey >= 0)
	{
		Outer = AddReferenceKey(OuterKey);
	}

	// 객체의 타입 찾기
	const FString& TypeName = Target["Type"];
	UClass* Type = UClass::FindByName(TypeName);

	// 새로 등록해야할 객체. 객체를 큐에 집어넣고 ID 발급
	Object = Existing;
	if (Object == nullptr)
	{
		Object = NewObject<UObject>(Outer, Type);
	}

	ReferenceTable[Reference] = Object;
	NextQueue.push(Reference);

	if (!Existing)
	{
		Object->Initialize();
	}
	return Object;
}

UObject* FJsonDataReader::GetReferenceKey(int32 Reference)
{
	auto It = ReferenceTable.find(Reference);

	if (It == ReferenceTable.end())
	{
		return nullptr;
	}

	return It->second;
}

FArchiveSection* FJsonDataReader::CurrentSection()
{
	if (!SectionStack.empty())
	{
		return &SectionStack.top();
	}
	else
	{
		return nullptr;
	}
}

nlohmann::json* FJsonDataReader::GetCurrentNode(FStringView Key)
{
	FArchiveSection* Section = CurrentSection();

	nlohmann::json& Node = *Section->Json;

	if (Section->Type == EArchiveSection::Array)
	{
		return &Node[Section->Index++];
	}

	return &Node[Key];
}
