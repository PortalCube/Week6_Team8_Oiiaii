#pragma once

#include "Runtime/Engine/Types/EngineTypes.h"
#include "Runtime/Asset/UPackage.h"
#include "Runtime/Core/Globals.h"
#include "FUObjectArray.h"
#include <concepts>

// TODO: 참조를 확실하게 관리하려면 TObjectPtr<TObject>를 반환하도록 바꿔야 함
template <UObjectType T>
T* NewObject(UObject* Outer)
{
	// 생성자로 오브젝트 생성
	T* Object = new T(Outer);

	// 전역 객체에 등록
	try
	{
		FUObjectArray::Get().AddObject(Object);
	}
	catch (...)
	{
		delete Object;
		throw;
	}

	return Object;
}

template <UObjectType T>
T* NewObject(UObject* Outer, UClass* ClassType)
{
	UObject* Object = ClassType->Create(Outer);

	T* TargetObject = Object->Cast<T>();
	return TargetObject;
}

/// <summary>
/// UObject를 엔진에서 안전하게 할당 해제합니다. (delete Object와 동일)
/// 제거된 UObject 포인터는 반드시 폐기해주세요.
/// </summary>
/// <param name="Object"></param>
inline void DestroyObject(UObject* Object)
{
	FUObjectArray& ObjectArray = FUObjectArray::Get();
	ObjectArray.DestroyObject(Object);
}

inline UPackage* GetTransientPackage()
{
	return Globals::TransientPackage;
}
