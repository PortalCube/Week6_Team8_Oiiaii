#include "UClass.h"
#include "Runtime/Engine/Types/PointerTypes.h"

UClass* UClass::RegisterToFactory(
    const FString& ClassName,
    const FString& SuperClassTypeName,
	UClass* SuperClass,
    const TFunction<UObject*(UObject*)>& CreateFunction)
{
	// 새로운 UClass 생성
	TUniquePtr<UClass> ClassType = MakeUnique<UClass>();
	ClassType->ClassName = ClassName;
	ClassType->SuperClassTypeName = SuperClassTypeName;
	ClassType->SuperClass = SuperClass;
	ClassType->CreateFunction = CreateFunction;

	UClass* Ptr = ClassType.get();

	// 배열 목록, Name 테이블에 등록
	ClassList.push_back(std::move(ClassType));
	NameTable[ClassName] = Ptr;

	return Ptr;
}

UObject* UClass::Create(UObject* Outer)
{
	UObject* Object = CreateFunction(Outer);

	return Object;
}

UClass* UClass::FindByName(const FString& Name)
{
	auto It = NameTable.find(Name);

	if (It != NameTable.end())
	{
		return It->second;
	}

	return nullptr;
}

UClass* UClass::FindClassWithDisplayName(const FString& Name)
{
	auto It = DisplayNameTable.find(Name);

	if (It != DisplayNameTable.end())
	{
		return It->second;
	}

	return nullptr;
}

const FString& UClass::GetDisplayName() const
{
	auto It = MetadataTable.find("DisplayName");

	if (It != MetadataTable.end())
	{
		return It->second;
	}

	return "";
}

void UClass::SetMeta(const FString& Key, const FString& Value)
{
	MetadataTable[Key] = Value;

	if (Key == "DisplayName")
	{
		DisplayNameTable[Value] = this;
	}
}
