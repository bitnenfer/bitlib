#pragma once

#include <bit/core/memory/allocator.h>
#include <bit/core/types.h>

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

	template<size_t Size>
	struct ConstFromKiB { static constexpr double Value = (double)Size / 1024.0; };

	template<size_t Size>
	struct ConstFromMiB { static constexpr double Value = ConstFromKiB<Size>::Value / 1024.0; };

	template<size_t Size>
	struct ConstFromGiB { static constexpr double Value = ConstFromMiB<Size>::Value / 1024.0; };

	template<size_t Size>
	struct ConstFromTiB { static constexpr double Value = ConstFromGiB<Size>::Value / 1024.0; };

	BITLIB_API void* Malloc(size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT);
	BITLIB_API void* Realloc(void* Pointer, size_t Size, size_t Alignment = bit::DEFAULT_ALIGNMENT);
	BITLIB_API void Free(void* Pointer);

	template<typename T>
	T* Malloc(size_t Count = 1)
	{
		return (T*)Malloc(Count * sizeof(T), alignof(T));
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
