#pragma once

#include <bit/types.h>
#include <bit/allocator.h>

#define KiB * (1024ULL)
#define MiB * (1024ULL KiB)
#define GiB * (1024ULL MiB)
#define TiB * (1024ULL MiB)

namespace bit
{
	struct IAllocator;

	BITLIB_API IAllocator& GetDefaultAllocator();
	BITLIB_API void* Memcpy(void* Dst, const void* Src, size_t Num);
	BITLIB_API void* Memset(void* Ptr, int32_t Value, size_t Num);
	BITLIB_API bool Memcmp(const void* A, const void* B, size_t Num);
	BITLIB_API size_t Strlen(const char* Str);
	BITLIB_API bool StrContains(const char* A, const char* B, size_t* Offset);
	BITLIB_API bool Strcmp(const char* A, const char* B);
	BITLIB_API size_t Fmt(char* Buffer, size_t BufferSize, const char* Fmt, ...);
	BITLIB_API const char* TempFmtString(const char* Fmt, ...);
	BITLIB_API double FromKiB(size_t Value);
	BITLIB_API double FromMiB(size_t Value);
	BITLIB_API double FromGiB(size_t Value);
	BITLIB_API double FromTiB(size_t Value);
	BITLIB_API void* AllocateFromOS(size_t Size);
	BITLIB_API bool FreeFromOS(void* Ptr);

	template<size_t Size>
	struct TFromKiB { static constexpr double Value = (double)Size / 1024.0; };

	template<size_t Size>
	struct TFromMiB { static constexpr double Value = TFromKiB<Size>::Value / 1024.0; };

	template<size_t Size>
	struct TFromGiB { static constexpr double Value = TFromMiB<Size>::Value / 1024.0; };

	template<size_t Size>
	struct TFromTiB { static constexpr double Value = TFromTiB<Size>::Value / 1024.0; };

	BIT_FORCEINLINE void* Malloc(size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT)
	{
		return GetDefaultAllocator().Allocate(Size, Alignment);
	}
	BIT_FORCEINLINE void* Realloc(void* Pointer, size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT)
	{
		return GetDefaultAllocator().Reallocate(Pointer, Size, Alignment);
	}
	BIT_FORCEINLINE void Free(void* Pointer)
	{
		return GetDefaultAllocator().Free(Pointer);
	}
	template<typename T>
	T* Malloc(size_t Count = 1)
	{
		return (T*)GetDefaultAllocator().Allocate(Count * sizeof(T), alignof(T));
	}

	template<class T>
	T* DefaultConstruct(T* Elements, size_t Count)
	{
		for (size_t Index = 0; Index < Count; ++Index)
		{
			T* Elem = &Elements[Index];
			BitPlacementNew(Elem) T();
		}
		return Elements;
	}

	template<typename T, typename... TArgs>
	T* ConstructArray(T* Elements, size_t Count, TArgs&& ... ConstructorArgs)
	{
		for (size_t Index = 0; Index < Count; ++Index)
		{
			T* Ptr = &Elements[Index];
			BitPlacementNew(Ptr) T(ConstructorArgs...);
		}
		return Elements;
	}

	template<class T>
	void DestroyArray(T* Elements, size_t Count)
	{
		for (size_t Index = 0; Index < Count; ++Index)
		{
			Elements[Index].~T();
		}
	}

	template<typename T, typename... TArgs>
	T* Construct(T* Element, TArgs&& ... ConstructorArgs)
	{
		return BitPlacementNew(Element) T(bit::Forward<TArgs>(ConstructorArgs)...);
	}

	template<class T>
	void Destroy(T* Element)
	{
		Element->~T();
	}

	template<typename T, typename... TArgs>
	T* New(TArgs&& ... ConstructorArgs)
	{
		return BitPlacementNew((T*)bit::Malloc(sizeof(T), alignof(T))) T(bit::Forward<TArgs>(ConstructorArgs)...);
	}

	template<typename T>
	void Delete(T* Ptr)
	{
		Ptr->~T();
		bit::Free(Ptr);
	}
}

//template<typename T>
//bool operator==(const T& LHS, const T& RHS)
//{
//	BIT_IF_CONSTEXPR(sizeof(T) <= 8) return LHS == RHS;
//	else if (&RHS == &LHS) return true;
//	else return bit::Memcmp(&LHS, &RHS, sizeof(T));
//}
