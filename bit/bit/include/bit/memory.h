#pragma once

#include <bit/types.h>

struct _BitMemNewDummy_ {};
inline void* operator new(size_t, _BitMemNewDummy_, void* Ptr) { return Ptr; }
inline void operator delete(void*, _BitMemNewDummy_, void*) {}
#define BitPlacementNew(Ptr) new(_BitMemNewDummy_(), Ptr)

namespace bit
{
	struct IAllocator;

	BIT_API IAllocator& GetDefaultAllocator();
	BIT_API void* Memcpy(void* Dst, const void* Src, size_t Num);
	BIT_API void* Memset(void* Ptr, int32_t Value, size_t Num);
	BIT_API int Memcmp(const void* A, const void* B, size_t Num);
	BIT_API size_t Fmt(char* Buffer, size_t BufferSize, const char* Fmt, ...);
	BIT_API const char* TempFmtString(const char* Fmt, ...);
	BIT_API size_t ToKiB(size_t Bytes);
	BIT_API size_t ToMiB(size_t Bytes);
	BIT_API size_t ToGiB(size_t Bytes);
	BIT_API size_t ToTiB(size_t Bytes);

	template<class T>
	void DefaultConstruct(T* Elements, size_t Count)
	{
		for (size_t Index = 0; Index < Count; ++Index)
		{
			T* Elem = &Elements[Index];
			BitPlacementNew(Elem) T();
		}
	}

	template<class T>
	void Destroy(T* Elements, size_t Count)
	{
		for (size_t Index = 0; Index < Count; ++Index)
		{
			Elements[Index].~T();
		}
	}


}
