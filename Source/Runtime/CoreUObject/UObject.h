#pragma once
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Engine/Types/EngineTypes.h"
#include "Runtime/Engine/Types/IntTypes.h"
#include <cstddef>
#include <new>
#include <concepts>

class UObjectGlobals;
class UClass;
class FArchive;

// 참고자료
// https://dev.epicgames.com/documentation/unreal-engine/objects-in-unreal-engine

// 레거시. 원래 언리얼은 UHT가 생성한 코드를 위해 넣어주는데 지금 그런게 없음
#define GENERATED_BODY()

/*
 * 역직렬화를 위한 타입 등록 매크로.
 *
 * 파일에는 타입이 "Cube" 같은 문자열로만 남는다. C++에서는 문자열로
 * new 를 호출할 수 없으므로, 클래스마다 자기 자신을 생성하는 함수를
 * 미리 만들어 두고 이름과 짝지어 등록해 둔다. 로드할 때는 그 이름으로
 * 등록된 생성 함수를 찾아 호출한다.
 *
 * 사용법
 *   헤더 : 클래스 본문 안에 DECLARE_UCLASS(UCubeComp, UPrimitiveComponent)
 *   cpp  : 파일 어딘가에 IMPLEMENT_UCLASS(UCubeComp, UPrimitiveComponent)
 *
 * UObject 는 부모가 없어 ROOT 버전을 쓴다. 프로젝트에서 ROOT 버전은
 * UObject 한 곳에서만 사용한다.
 */

////////////////////////////////////////////////////////////
// UObject의 Class 정의 매크로
////////////////////////////////////////////////////////////
#define DECLARE_ROOT_UCLASS(ClassName)                \
                                                      \
protected:                                            \
	ClassName(UObject* InOuter);                      \
                                                      \
	ClassName() = default;                            \
	virtual ~ClassName() = default;                   \
                                                      \
public:                                               \
	ClassName(const ClassName&) = delete;             \
	ClassName(const ClassName&&) = delete;            \
	ClassName& operator=(const ClassName&) = delete;  \
	ClassName& operator=(const ClassName&&) = delete; \
                                                      \
public:                                               \
	virtual UClass* GetClass() const;                 \
	static UClass* StaticClass();                     \
                                                      \
private:                                              \
	static ClassName* CreateObject(UObject* Outer);   \
                                                      \
	static inline UClass* ClassInfo =                 \
	    UClass::RegisterToFactory(                    \
	        #ClassName,                               \
	        "",                                       \
	        nullptr,                                  \
	        &ClassName::CreateObject);                \
                                                      \
	template <UObjectType TObject>                    \
	friend TObject* NewObject(UObject* Outer);

////////////////////////////////////////////////////////////
// UObject의 Class 구현 매크로
////////////////////////////////////////////////////////////
#define IMPLEMENT_ROOT_UCLASS(ClassName)             \
	ClassName::ClassName(UObject* InOuter)           \
	    : Outer{ InOuter }                           \
	{                                                \
	}                                                \
                                                     \
	UObject* ClassName::CreateObject(UObject* Outer) \
	{                                                \
		return NewObject<ClassName>(Outer);          \
	}                                                \
                                                     \
	UClass* ClassName::StaticClass()                 \
	{                                                \
		return ClassInfo;                            \
	}                                                \
                                                     \
	UClass* ClassName::GetClass() const              \
	{                                                \
		return StaticClass();                        \
	}

////////////////////////////////////////////////////////////
// 자식 클래스의 Class 정의 매크로
////////////////////////////////////////////////////////////
#define DECLARE_UCLASS(ClassName, ParentClass)        \
protected:                                            \
	ClassName(UObject* InOuter);                      \
                                                      \
	ClassName() = default;                            \
	virtual ~ClassName() = default;                   \
                                                      \
public:                                               \
	ClassName(const ClassName&) = delete;             \
	ClassName(const ClassName&&) = delete;            \
	ClassName& operator=(const ClassName&) = delete;  \
	ClassName& operator=(const ClassName&&) = delete; \
                                                      \
public:                                               \
	UClass* GetClass() const override;                \
	static UClass* StaticClass();                     \
	using Super = ParentClass;                        \
                                                      \
private:                                              \
	static UObject* CreateObject(UObject* Outer);     \
                                                      \
	static inline UClass* ClassInfo =                 \
	    UClass::RegisterToFactory(                    \
	        #ClassName,                               \
	        #ParentClass,                             \
	        ParentClass::StaticClass(),               \
	        &ClassName::CreateObject);                \
                                                      \
	template <UObjectType TObject>                    \
	friend TObject* NewObject(UObject* Outer);

////////////////////////////////////////////////////////////
// 자식 클래스의 Class 구현 매크로
////////////////////////////////////////////////////////////
#define IMPLEMENT_UCLASS(ClassName, ParentClass)     \
	ClassName::ClassName(UObject* InOuter)           \
	    : ParentClass{ InOuter }                     \
	{                                                \
	}                                                \
                                                     \
	UObject* ClassName::CreateObject(UObject* Outer) \
	{                                                \
		return NewObject<ClassName>(Outer);          \
	}                                                \
                                                     \
	UClass* ClassName::StaticClass()                 \
	{                                                \
		return ClassInfo;                            \
	}                                                \
                                                     \
	UClass* ClassName::GetClass() const              \
	{                                                \
		return StaticClass();                        \
	}

////////////////////////////////////////////////////////////
// UClass에 메타데이터 추가 매크로
////////////////////////////////////////////////////////////
#define UCLASS_META(ClassName, Key, Value)                  \
	struct _MetaRegister_##ClassName##_##Key                \
	{                                                       \
		_MetaRegister_##ClassName##_##Key()                 \
		{                                                   \
			ClassName::StaticClass()->SetMeta(#Key, Value); \
		}                                                   \
	} _MetaRegisterInstance_##ClassName##_##Key;

class UObject
{
	GENERATED_BODY()
	DECLARE_ROOT_UCLASS(UObject)

	friend class FUObjectArray;

public:
	////////////////////////////////////////////////////////////
	// UUID
	////////////////////////////////////////////////////////////

	uint32 UUID = 0u;
	uint32 InternalIndex = 0u;

	uint32 GetUUID() const { return UUID; }
	void SetUUID(uint32 InUUID) { UUID = InUUID; }

	////////////////////////////////////////////////////////////
	// 생명 주기
	////////////////////////////////////////////////////////////

	virtual void Initialize();
	virtual void Release();

	////////////////////////////////////////////////////////////
	// Memory Allocator
	////////////////////////////////////////////////////////////

	static void* operator new(std::size_t Size);
	static void* operator new(std::size_t Size, std::align_val_t Alignment);

	static void operator delete(void* Memory, std::size_t Size) noexcept;
	static void operator delete(void* Memory, std::size_t Size, std::align_val_t Alignment) noexcept;

	static void* operator new[](std::size_t) = delete;
	static void operator delete[](void*) = delete;

	static inline uint64 TotalAllocationBytes = 0;
	static inline uint64 TotalAllocationCount = 0;

	static uint64 GetTotalAllocationBytes()
	{
		return TotalAllocationBytes;
	}

	static uint64 GetTotalAllocationCount()
	{
		return TotalAllocationCount;
	}

	////////////////////////////////////////////////////////////
	// 직렬화 / 역직렬화
	////////////////////////////////////////////////////////////

	virtual void Serialize(FArchive& Archive);

	////////////////////////////////////////////////////////////
	// CreateDefaultSubobject
	////////////////////////////////////////////////////////////

protected:
	template <UObjectType T>
	T* CreateDefaultSubobject();

	template <UObjectType T>
	T* CreateEditorOnlyDefaultSubobject();

	////////////////////////////////////////////////////////////
	// Editor Flag
	////////////////////////////////////////////////////////////

	virtual void MarkAsEditorOnlySubobject() {}
	virtual bool IsEditorOnly() const;

	////////////////////////////////////////////////////////////
	// Editor Flag
	////////////////////////////////////////////////////////////

	bool bIsExternal = false;

	////////////////////////////////////////////////////////////
	// 상속 관계
	////////////////////////////////////////////////////////////

public:
	// 이 UObject를 가지고 있는 객체를 가리킵니다.
	UObject* Outer = nullptr;

	// UObject를 가지고 있는 객체를 반환합니다.
	UObject* GetOuter() const;

	template <UObjectType T>
	T* GetTypedOuter() const;

	UObject* GetTypedOuter(UClass* Class) const;

	virtual class UWorld* GetWorld() const;

	////////////////////////////////////////////////////////////
	// Type Check
	////////////////////////////////////////////////////////////

	template <UObjectType T>
	bool IsA() const;

	bool IsA(UClass* ClassType) const;

	template <typename T>
	T* Cast();

	template <typename T>
	const T* Cast() const;
};

////////////////////////////////////////////////////////////
// 템플릿 함수 구현부
////////////////////////////////////////////////////////////

template <UObjectType T>
inline T* UObject::CreateDefaultSubobject()
{
	T* Object = NewObject<T>(this);
	Object->Initialize();

	return Object;
}

template <UObjectType T>
inline T* UObject::CreateEditorOnlyDefaultSubobject()
{
	T* EditorSubobject = CreateDefaultSubobject<T>();

	if (EditorSubobject)
	{
		EditorSubobject->MarkAsEditorOnlySubobject();
	}

	return EditorSubobject;
}

template <UObjectType T>
inline T* UObject::GetTypedOuter() const
{
	return static_cast<T*>(GetTypedOuter(T::StaticClass()));
}

template <UObjectType T>
inline bool UObject::IsA() const
{
	return IsA(T::StaticClass());
}

template <typename T>
inline T* UObject::Cast()
{
	return IsA<T>() ? static_cast<T*>(this) : nullptr;
}

template <typename T>
inline const T* UObject::Cast() const
{
	return IsA<T>() ? static_cast<const T*>(this) : nullptr;
}
