#pragma once
#include "Runtime/Core/TFunction.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Engine/Types/IntTypes.h"
#include "Runtime/Engine/Types/PointerTypes.h"
#include "Runtime/Core/TMap.h"

class UObject;

class UClass
{
private:
	// 현재 존재하는 모든 클래스의 배열입니다.
	static inline TArray<TUniquePtr<UClass>> ClassList;

	// 클래스 이름 테이블
	static inline TMap<FString, UClass*> NameTable;
	static inline TMap<FString, UClass*> DisplayNameTable;

	// 클래스 이름
	FString ClassName;
	FString SuperClassTypeName;

	// 메타데이터 테이블
	TMap<FString, FString> MetadataTable;

	// 생성자 함수
	TFunction<UObject*(UObject*)> CreateFunction;

	// 부모 클래스 포인터
	UClass* SuperClass;

public:

	////////////////////////////////////////////////////////////
	// Register 
	////////////////////////////////////////////////////////////

	// 주어진 타입 정보로 UClass를 만들고 레지스트리에 등록합니다.
	static UClass* RegisterToFactory(
	    const FString& ClassName,
	    const FString& SuperClassTypeName,
		UClass* SuperClass,
	    const TFunction<UObject*(UObject*)>& CreateFunction);

	UObject* Create(UObject* Outer);



	////////////////////////////////////////////////////////////
	// Get Name, Find Name
	////////////////////////////////////////////////////////////
	
	const FString& GetName() const { return ClassName; }
	const FString& GetDisplayName() const;

	static UClass* FindByName(const FString& Name);
	static UClass* FindClassWithDisplayName(const FString& Name);



	////////////////////////////////////////////////////////////
	// Getter, Setter
	////////////////////////////////////////////////////////////

	// 메타 정보를 지정합니다.
	void SetMeta(const FString& Key, const FString& Value);

	// 부모 클래스 정보를 반환합니다.
	UClass* GetSuperClass() { return SuperClass; }

};
