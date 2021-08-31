#pragma once

#include <bit/types.h>
#include <bit/allocator.h>

namespace bit
{
	BITLIB_API IAllocator& GetDefaultAllocator();
	BITLIB_API void* Memcpy(void* Dst, const void* Src, size_t Num);
	BITLIB_API void* Memset(void* Ptr, int32_t Value, size_t Num);
	BITLIB_API int Memcmp(const void* A, const void* B, size_t Num);
	BITLIB_API size_t Strlen(const char* Str);
	BITLIB_API bool StrContains(const char* A, const char* B, size_t* Offset);
	BITLIB_API bool Strcmp(const char* A, const char* B);
	BITLIB_API size_t Fmt(char* Buffer, size_t BufferSize, const char* Fmt, ...);
	BITLIB_API const char* TempFmtString(const char* Fmt, ...);
	BITLIB_API size_t ToKiB(size_t Value);
	BITLIB_API size_t ToMiB(size_t Value);
	BITLIB_API size_t ToGiB(size_t Value);
	BITLIB_API size_t ToTiB(size_t Value);

	BIT_FORCEINLINE void* Malloc(size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT)
	{
		return GetDefaultAllocator().Alloc(Size, Alignment);
	}
	BIT_FORCEINLINE void* Realloc(void* Pointer, size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT)
	{
		return GetDefaultAllocator().Realloc(Pointer, Size, Alignment);
	}
	BIT_FORCEINLINE void Free(void* Pointer)
	{
		return GetDefaultAllocator().Free(Pointer);
	}
	template<typename T>
	T* TMalloc(size_t Count = 1)
	{
		return (T*)GetDefaultAllocator().Alloc(Count * sizeof(T), bit::DEFAULT_ALIGNMENT);
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
	T* Construct(T* Elements, size_t Count, TArgs&& ... ConstructorArgs)
	{
		for (size_t Index = 0; Index < Count; ++Index)
		{
			T* Ptr = &Elements[Index];
			BitPlacementNew(Ptr) T(ConstructorArgs...);
		}
		return Elements;
	}

	template<class T>
	void Destroy(T* Elements, size_t Count)
	{
		for (size_t Index = 0; Index < Count; ++Index)
		{
			Elements[Index].~T();
		}
	}

	template<typename T, typename... TArgs>
	T* New(TArgs&& ... ConstructorArgs)
	{
		return BitPlacementNew((T*)bit::Malloc(sizeof(T), 4)) T(ConstructorArgs...);
	}

	template<typename T>
	void Delete(T* Ptr)
	{
		Ptr->~T();
		bit::Free(Ptr);
	}
}

template<typename T>
bool operator==(const T& LHS, const T& RHS)
{
	BIT_IF_CONSTEXPR(sizeof(T) <= 8) return LHS == RHS;
	else if (&RHS == &LHS) return true;
	else return bit::Memcmp(&LHS, &RHS, sizeof(T));
}