#include "UObject.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Core/FMemory.h"

IMPLEMENT_ROOT_UCLASS(UObject)
UCLASS_META(UObject, DisplayName, "Object")

void UObject::Initialize()
{
}

void UObject::Release()
{
}

void UObject::Serialize(FArchive& Archive) const
{
	Archive.SetInt32("UUID", UUID);
	Archive.SetString("Type", GetClass()->GetName());
}

void UObject::Deserialize(const FArchive& Archive)
{
	UUID = Archive.GetInt32("UUID");
}

UObject* UObject::GetOuter() const
{
	return Outer;
}

class UWorld* UObject::GetWorld() const
{
	// 부모의 GetWorld()를 재귀적으로 호출하여 반환
	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}

	// 부모가 없으면 nullptr
	return nullptr;
}

bool UObject::IsA(UClass* ClassType) const
{
	UClass* CurrentInfo = GetClass();

	while (CurrentInfo)
	{
		if (CurrentInfo == ClassType)
		{
			return true;
		}

		CurrentInfo = CurrentInfo->GetSuperClass();
	}

	return false;
}

void* UObject::operator new(std::size_t Size)
{
	// void* Memory = ::operator new(Size);

	void* Memory = FMemory::Malloc(Size);

	TotalAllocationBytes += Size;
	++TotalAllocationCount;

	return Memory;
}

void UObject::operator delete(void* Memory, std::size_t Size) noexcept
{
	if (Memory == nullptr)
		return;

	TotalAllocationBytes -= Size;
	--TotalAllocationCount;

	//::operator delete(Memory);

	FMemory::Free(Memory);
}

void* UObject::operator new(std::size_t Size, std::align_val_t Alignment)
{
	// void* Memory = ::operator new(Size, Alignment);
	void* Memory = FMemory::Malloc(Size, static_cast<size_t>(Alignment));

	TotalAllocationBytes += Size;
	++TotalAllocationCount;

	return Memory;
}

void UObject::operator delete(void* Memory, std::size_t Size, std::align_val_t Alignment) noexcept
{
	if (Memory == nullptr)
		return;

	TotalAllocationBytes -= Size;
	--TotalAllocationCount;

	//::operator delete(Memory, Alignment);
	FMemory::Free(Memory, static_cast<size_t>(Alignment));
}
